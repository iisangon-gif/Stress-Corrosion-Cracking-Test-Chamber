// Closed-loop simulation: the real SCC_EnvControl firmware running against a
// lumped thermal / moisture model of a small chamber.
//
// Scenario (8 h): start 25 C / 60 %RH -> set 35 C / 80 %RH
//                 4 h: set 40 C / 70 %RH
//                 6 h: lid open for 60 s
// Checked windows: 1.5-4 h, 5.5-6 h, 6 h 15 min - 8 h  ->  +/-1 C, +/-2 %RH
//
// Plant parameters can be overridden with environment variables:
//   CAP  heat capacity J/K        (3000)   UA  wall loss W/K          (0.8)
//   PW   heater power W           (50)     HR  humidifier g/m3/s      (0.3)
//   TAU  sensor lag s             (10)     TA  room temperature C     (25)
//   RA   room RH fraction         (0.6)
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
  const double C = env("CAP", 3000), UA = env("UA", 0.8), Pmax = env("PW", 50);
  const double humRate = env("HR", 0.30), tau = env("TAU", 10);
  const double Tamb = env("TA", 25), AHamb = env("RA", 0.6) * ahSat(Tamb);
  const double UAvent = 3.0, kLeak = 0.002, kVent = 0.02;   // fan venting / leakage
  const double dt = 0.1, hours = 8;

  setup();
  const unsigned long base = g_millis;
  Serial.in = "SET T 35\nSET RH 80\nSET LOG 30\n";

  double T = Tamb, AH = AHamb, ts = T, rhs = 100 * AH / ahSat(T);
  double maxDT[3] = {0}, maxDH[3] = {0};
  long n[3] = {0}, inBand[3] = {0};

  for (long i = 0; i < hours * 3600 / dt; i++) {
    double t = i * dt;
    g_millis = base + (unsigned long)(t * 1000);
    if (i == (long)(4 * 3600 / dt)) Serial.in += "SET T 40\nSET RH 70\n";
    bool lid = t > 6 * 3600 && t < 6 * 3600 + 60;

    double P = Pmax * g_pwm[PIN_HEATER] / 255.0;
    double vent = g_pwm[PIN_EXHAUST] / 255.0;
    bool hum = g_pins[PIN_HUMIDIFIER] == HIGH;

    double ua = UA + vent * UAvent + (lid ? 20 : 0);
    T += dt * (P - ua * (T - Tamb)) / C;
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
      double sp = t < 4 * 3600 ? 35 : 40, sh = t < 4 * 3600 ? 80 : 70;
      double eT = fabs(T - sp), eH = fabs(rh - sh);
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

  const char *name[3] = {"35C/80% steady", "40C/70% steady", "after lid opening"};
  bool ok = true;
  for (int s = 0; s < 3; s++) {
    printf("%-18s max|dT|=%.2f C  max|dRH|=%.2f %%  in-band=%.1f%%\n",
           name[s], maxDT[s], maxDH[s], 100.0 * inBand[s] / n[s]);
    ok = ok && inBand[s] == n[s];
  }
  printf("%s\n", ok ? "PASS" : "FAIL");
  return ok ? 0 : 1;
}
