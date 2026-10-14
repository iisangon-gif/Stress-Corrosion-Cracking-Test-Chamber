// Closed-loop simulation: the real SCC_EnvControl firmware running against a
// lumped thermal / moisture model of a small chamber.
//
// Scenario (8 h): start at room conditions -> set-point 1 (T1 / H1)
//                 4 h: set-point 2 (T2 / H2)
//                 6 h: lid open for 60 s
// Checked windows: 1.5-4 h, 5.5-6 h, 6 h 15 min - 8 h  ->  +/-1 C, +/-2 %RH
// (T1/T2 may be "ROOM": no heating, temperature not checked)
//
// The heater is a pad on the OUTSIDE of the chamber wall (kept out of the
// salt fog), so heat reaches the air through a wall node:
//   heater -> wall (CW, J/K) -> air (CAP, J/K) -> room
//
// Plant parameters can be overridden with environment variables:
//   CAP  air + contents heat capacity J/K (3000)  UA  air loss to room W/K (0.8)
//   CW   heated wall heat capacity J/K    (800)   GW  wall -> air W/K       (4)
//   PW   heater power W                   (100)   HR  4 mist heads g/m3/s  (0.7)
//   TAU  sensor lag s                     (10)    TA  room temperature C    (25)
//   RA   room RH fraction                 (0.6)   MD  mist start delay s     (0.3)
//   T1 H1 / T2 H2  set-points             (35 / 70, 40 / 40)
//   CSV  write the firmware's serial CSV output to this file
//
// Exit code 0 = all windows within tolerance.
#include "stubs/firmware.h"

unsigned long g_millis = 0;
int g_pins[64], g_pwm[64];
SerialT Serial;
EEPROMClass EEPROM;
float g_tSens = 25, g_rhSens = 60;
bool g_dhtFail = false;
uint8_t g_buttons = 0;
char g_lcd[2][17];

static double env(const char *k, double def) {
  const char *v = getenv(k);
  return v ? atof(v) : def;
}

// saturation absolute humidity, g/m3 (Magnus)
static double ahSat(double T) {
  double es = 6.112 * exp(17.62 * T / (243.12 + T));
  return 216.7 * es / (273.15 + T);
}

int main() {
  const double C = env("CAP", 3000), UA = env("UA", 0.8), Pmax = env("PW", 100);
  const double Cw = env("CW", 800), Gw = env("GW", 4), UAw = 0.3;  // wall node
  const double humRate = env("HR", 0.7), tau = env("TAU", 10);
  const double Tamb = env("TA", 25), AHamb = env("RA", 0.6) * ahSat(Tamb);
  const double UAvent = 3.0, kLeak = 0.002, kVent = 0.02;   // fan venting / leakage
  const double dt = 0.1, hours = 8, mistDelay = env("MD", 0.3);
  const char *T1 = getenv("T1") ? getenv("T1") : "35", *T2 = getenv("T2") ? getenv("T2") : "40";
  const double H1 = env("H1", 70), H2 = env("H2", 40);
  const bool room1 = !strcasecmp(T1, "ROOM"), room2 = !strcasecmp(T2, "ROOM");
  const double sp1 = room1 ? 0 : atof(T1), sp2 = room2 ? 0 : atof(T2);
  char cmd[64];

  setup();
  const unsigned long base = g_millis;
  snprintf(cmd, sizeof(cmd), "SET T %s\nSET RH %g\nSET LOG 30\n", T1, H1);
  Serial.in = cmd;
  double mistOnFor = 0;

  double T = Tamb, Tw = Tamb, AH = AHamb, ts = T, rhs = 100 * AH / ahSat(T);
  double maxDT[3] = {0}, maxDH[3] = {0};
  long n[3] = {0}, inBand[3] = {0};

  for (long i = 0; i < hours * 3600 / dt; i++) {
    double t = i * dt;
    g_millis = base + (unsigned long)(t * 1000);
    if (i == (long)(4 * 3600 / dt)) {
      snprintf(cmd, sizeof(cmd), "SET T %s\nSET RH %g\n", T2, H2);
      Serial.in += cmd;
    }
    bool lid = t > 6 * 3600 && t < 6 * 3600 + 60;

    double P = g_pins[PIN_HEATER] == HIGH ? Pmax : 0;
    double vent = g_pins[PIN_VENT] == HIGH ? 1 : 0;
    // mist starts mistDelay s after the output switches on (USB adapter + heads)
    mistOnFor = g_pins[PIN_MIST] == HIGH ? mistOnFor + dt : 0;
    bool hum = mistOnFor > mistDelay;

    double ua = UA + vent * UAvent + (lid ? 20 : 0);
    double qWall = Gw * (Tw - T);
    Tw += dt * (P - qWall - UAw * (Tw - Tamb)) / Cw;
    T += dt * (qWall - ua * (T - Tamb)) / C;
    double k = kLeak + vent * kVent + (lid ? 0.05 : 0);
    AH += dt * ((hum ? humRate : 0) - k * (AH - AHamb));
    if (AH > ahSat(T)) AH = ahSat(T);
    double rh = 100 * AH / ahSat(T);

    ts += dt * (T - ts) / tau;            // sensor lag
    rhs += dt * (rh - rhs) / tau;
    g_tSens = ts + 0.05 * sin(t * 0.37);  // small noise
    g_rhSens = rhs + 0.3 * sin(t * 0.21);

    loop();

    int seg = -1;
    if (t > 1.5 * 3600 && t < 4 * 3600) seg = 0;
    else if (t > 5.5 * 3600 && t < 6 * 3600) seg = 1;
    else if (t > 6 * 3600 + 900) seg = 2;
    if (seg >= 0 && i % 10 == 0) {
      bool first = t < 4 * 3600, room = first ? room1 : room2;
      double sp = first ? sp1 : sp2, sh = first ? H1 : H2;
      double eT = room ? 0 : fabs(T - sp), eH = fabs(rh - sh);
      if (eT > maxDT[seg]) maxDT[seg] = eT;
      if (eH > maxDH[seg]) maxDH[seg] = eH;
      n[seg]++;
      if (eT <= T_TOL && eH <= RH_TOL) inBand[seg]++;
    }
  }

  if (const char *path = getenv("CSV")) {
    FILE *f = fopen(path, "w");
    if (f) { fputs(Serial.out.c_str(), f); fclose(f); }
  }

  char name[3][32];
  snprintf(name[0], 32, "%s C / %g %% steady", T1, H1);
  snprintf(name[1], 32, "%s C / %g %% steady", T2, H2);
  snprintf(name[2], 32, "after lid opening");
  bool ok = true;
  for (int s = 0; s < 3; s++) {
    printf("%-24s max|dT|=%.2f C  max|dRH|=%.2f %%  in-band=%.1f%%\n",
           name[s], maxDT[s], maxDH[s], 100.0 * inBand[s] / n[s]);
    ok = ok && inBand[s] == n[s];
  }
  printf("%s\n", ok ? "PASS" : "FAIL");
  return ok ? 0 : 1;
}
