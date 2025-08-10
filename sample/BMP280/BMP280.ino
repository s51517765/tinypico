// BMP280 スレーブアドレス (SDOピンをGNDに接続した場合)
// SDOピンをVCCに接続した場合は 0x77
#define BMP280_ADDRESS 0x76

// BMP280 レジスタアドレス
#define BMP280_REG_CALIB00 0x88 // 補正値データレジスタ開始アドレス
#define BMP280_REG_CHIPID  0xD0 // チップIDレジスタ
#define BMP280_REG_RESET   0xE0 // ソフトリセットレジスタ
#define BMP280_REG_STATUS  0xF3 // ステータスレジスタ
#define BMP280_REG_CONFIG  0xF5 // 設定レジスタ
#define BMP280_REG_CTRL_MEAS 0xF4 // 制御・測定レジスタ
#define BMP280_REG_PRESS_MSB 0xF7 // 圧力データレジスタ開始アドレス
#define BMP280_REG_TEMP_MSB  0xFA // 温度データレジスタ開始アドレス (BMP280はF7-F9が圧力, FA-FCが温度)

// I2C ピン定義 (任意のデジタルピンに設定可能)
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 4

// I2C タイミングのための遅延 (Arduino Uno/Nano/Mega @ 16MHz 想定)
// ATtinyシリーズなど、クロック速度が異なる場合は調整が必要な場合があります
#define I2C_DELAY_US 5 // マイクロ秒単位の遅延

// ---------------------------------------------------------------
// ビットバンギングによる基本的なI2C通信関数
// ---------------------------------------------------------------
// この部分はI2Cプロトコルをソフトウェアで制御しています
// コードサイズに影響しますが、Wireライブラリを使わないため必要です

static void i2c_init_pins() {
  pinMode(I2C_SDA_PIN, OUTPUT);
  pinMode(I2C_SCL_PIN, OUTPUT);
  digitalWrite(I2C_SDA_PIN, HIGH);
  digitalWrite(I2C_SCL_PIN, HIGH);
  delayMicroseconds(I2C_DELAY_US);
}

static void i2c_sda_input() {
  pinMode(I2C_SDA_PIN, INPUT);
}

static void i2c_sda_output() {
  pinMode(I2C_SDA_PIN, OUTPUT);
}

static void i2c_start() {
  i2c_sda_output();
  digitalWrite(I2C_SDA_PIN, HIGH);
  digitalWrite(I2C_SCL_PIN, HIGH);
  delayMicroseconds(I2C_DELAY_US);
  digitalWrite(I2C_SDA_PIN, LOW);
  delayMicroseconds(I2C_DELAY_US);
  digitalWrite(I2C_SCL_PIN, LOW);
  delayMicroseconds(I2C_DELAY_US);
}

static void i2c_stop() {
  i2c_sda_output();
  digitalWrite(I2C_SDA_PIN, LOW);
  delayMicroseconds(I2C_DELAY_US);
  digitalWrite(I2C_SCL_PIN, HIGH);
  delayMicroseconds(I2C_DELAY_US);
  digitalWrite(I2C_SDA_PIN, HIGH);
  delayMicroseconds(I2C_DELAY_US);
}

static bool i2c_write_byte(byte data) {
  i2c_sda_output();

  for (int i = 0; i < 8; i++) {
    if ((data & 0x80) == 0) {
      digitalWrite(I2C_SDA_PIN, LOW);
    } else {
      digitalWrite(I2C_SDA_PIN, HIGH);
    }
    data <<= 1;
    delayMicroseconds(I2C_DELAY_US);
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(I2C_DELAY_US);
    digitalWrite(I2C_SCL_PIN, LOW);
    delayMicroseconds(I2C_DELAY_US);
  }

  i2c_sda_input();
  delayMicroseconds(I2C_DELAY_US);
  digitalWrite(I2C_SCL_PIN, HIGH);
  delayMicroseconds(I2C_DELAY_US);
  bool ack = digitalRead(I2C_SDA_PIN) == LOW;

  digitalWrite(I2C_SCL_PIN, LOW);
  delayMicroseconds(I2C_DELAY_US);
  i2c_sda_output();

  return ack;
}

static byte i2c_read_byte(bool ack_out) {
  i2c_sda_input();

  byte data = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(I2C_DELAY_US);
    data <<= 1;
    if (digitalRead(I2C_SDA_PIN)) {
      data |= 1;
    }
    digitalWrite(I2C_SCL_PIN, LOW);
    delayMicroseconds(I2C_DELAY_US);
  }

  i2c_sda_output();
  if (ack_out) {
    digitalWrite(I2C_SDA_PIN, LOW);
  } else {
    digitalWrite(I2C_SDA_PIN, HIGH);
  }
  delayMicroseconds(I2C_DELAY_US);
  digitalWrite(I2C_SCL_PIN, HIGH);
  delayMicroseconds(I2C_DELAY_US);
  digitalWrite(I2C_SCL_PIN, LOW);
  delayMicroseconds(I2C_DELAY_US);
  i2c_sda_input();

  return data;
}

static bool bmp280_read_registers(byte reg_addr, byte *data, uint8_t num_bytes) {
  i2c_start();
  if (!i2c_write_byte((BMP280_ADDRESS << 1) | 0x00)) {
    i2c_stop();
    // Serial.println("I2C Write Address NACK"); // ATtiny向け: デバッグ出力を削減
    return false;
  }
  if (!i2c_write_byte(reg_addr)) {
    i2c_stop();
    // Serial.println("I2C Register Address NACK"); // ATtiny向け: デバッグ出力を削減
    return false;
  }
  i2c_start();
  if (!i2c_write_byte((BMP280_ADDRESS << 1) | 0x01)) {
    i2c_stop();
    // Serial.println("I2C Read Address NACK"); // ATtiny向け: デバッグ出力を削減
    return false;
  }

  for (int i = 0; i < num_bytes; i++) {
    bool send_ack = (i < num_bytes - 1);
    data[i] = i2c_read_byte(send_ack);
  }

  i2c_stop();
  return true;
}

static bool bmp280_write_register(byte reg_addr, byte data) {
  i2c_start();
  if (!i2c_write_byte((BMP280_ADDRESS << 1) | 0x00)) {
    i2c_stop();
    // Serial.println("I2C Write Address NACK"); // ATtiny向け: デバッグ出力を削減
    return false;
  }
  if (!i2c_write_byte(reg_addr)) {
    i2c_stop();
    // Serial.println("I2C Register Address NACK"); // ATtiny向け: デバッグ出力を削減
    return false;
  }
  if (!i2c_write_byte(data)) {
    i2c_stop();
    // Serial.println("I2C Data NACK"); // ATtiny向け: デバッグ出力を削減
    return false;
  }
  i2c_stop();
  return true;
}

// BMP280 補正値を格納する変数 (データシート Table 17)
uint16_t dig_T1;
int16_t dig_T2;
int16_t dig_T3;
uint16_t dig_P1;
int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;

// 補正計算用の中間変数 (データシート Section 3.11.3)
// 温度補正時に計算され、圧力補正に使用される
long t_fine;


// BMP280の補正値を読み出す
static bool bmp280_read_calibration() {
  byte calib_data[24]; // 0x88 から 24バイト
  if (!bmp280_read_registers(BMP280_REG_CALIB00, calib_data, 24)) {
    return false;
  }

  // 読み出したバイト列をデータシートに従って各補正値に格納 (Table 17)
  dig_T1 = (calib_data[1] << 8) | calib_data[0]; // uint16
  dig_T2 = (calib_data[3] << 8) | calib_data[2]; // int16
  dig_T3 = (calib_data[5] << 8) | calib_data[4]; // int16

  dig_P1 = (calib_data[7] << 8) | calib_data[6]; // uint16
  dig_P2 = (calib_data[9] << 8) | calib_data[8]; // int16
  dig_P3 = (calib_data[11] << 8) | calib_data[10]; // int16
  dig_P4 = (calib_data[13] << 8) | calib_data[12]; // int16
  dig_P5 = (calib_data[15] << 8) | calib_data[14]; // int16
  dig_P6 = (calib_data[17] << 8) | calib_data[16]; // int16
  dig_P7 = (calib_data[19] << 8) | calib_data[18]; // int16
  dig_P8 = (calib_data[21] << 8) | calib_data[20]; // int16
  dig_P9 = (calib_data[23] << 8) | calib_data[22]; // int16

  // Serial.println("Calibration data read."); // ATtiny向け: デバッグ出力を削減
  return true;
}

// BMP280を初期化し、設定を行う
static bool bmp280_init() {
  byte chip_id;
  if (!bmp280_read_registers(BMP280_REG_CHIPID, &chip_id, 1)) {
    //Serial.println("BMP280 read ID failed."); // 初期化失敗
    return false;
  }
  if (chip_id != 0x58) { // BMP280のチップIDは0x58
    //Serial.print("Wrong Chip ID: 0x"); Serial.println(chip_id, HEX); // 初期化失敗は重要なので残す
    return false;
  }
  // Serial.println("BMP280 found."); // ATtiny向け: デバッグ出力を削減

  // ソフトリセット (オプション)
  // bmp280_write_register(BMP280_REG_RESET, 0xB6); delay(100);

  if (!bmp280_read_calibration()) {
    //Serial.println("BMP280 read calibration failed."); // 初期化失敗は重要なので残す
    return false;
  }

  // CONFIG: 0x00 (t_sb=0.5ms, filter=off, spi3w_en=0)
  if (!bmp280_write_register(BMP280_REG_CONFIG, 0x00)) {
    //Serial.println("BMP280 write config failed."); // 初期化失敗は重要なので残す
    return false;
  }

  // CTRL_MEAS: 0x25 (osrs_t=x1, osrs_p=x1, mode=Forced)
  if (!bmp280_write_register(BMP280_REG_CTRL_MEAS, 0x25)) {
    //Serial.println("BMP280 write ctrl_meas failed."); // 初期化失敗は重要なので残す
    return false;
  }

  // Serial.println("BMP280 initialized."); // ATtiny向け: デバッグ出力を削減
  return true;
}

// BMP280の生の温度と圧力データを読み出す
// 生データは20ビット値
static uint32_t* bmp280_read_raw_data() {
  static uint32_t raw_data[2] = {0, 0}; // pressure, temperature
  byte data[6];

  // Forced mode: 測定完了を待つ
  byte status;
  unsigned long start_time = millis();
  const unsigned long timeout = 500;

  do {
    delay(1);
    if (!bmp280_read_registers(BMP280_REG_STATUS, &status, 1)) {
      // Serial.println("Failed to read status."); // ATtiny向け: デバッグ出力を削減
      raw_data[0] = 0; raw_data[1] = 0;
      return raw_data;
    }
    if (millis() - start_time > timeout) {
      //Serial.println("Measurement timeout."); // タイムアウト
      raw_data[0] = 0; raw_data[1] = 0;
      return raw_data;
    }
  } while ((status & 0x08) != 0); // measuringビットが立っている間は待機


  if (!bmp280_read_registers(BMP280_REG_PRESS_MSB, data, 6)) {
    // Serial.println("Failed to read raw data."); // ATtiny向け: デバッグ出力を削減
    raw_data[0] = 0; raw_data[1] = 0;
    return raw_data;
  }

  // 生データを20ビット値に変換
  raw_data[0] = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | (data[2] >> 4); // 圧力 raw_P (20bit)
  raw_data[1] = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | (data[5] >> 4); // 温度 raw_T (20bit)

  // Forced mode なので、次の測定のために再度 CTRL_MEAS レジスタに書き込む
  if (!bmp280_write_register(BMP280_REG_CTRL_MEAS, 0x25)) {
    // Serial.println("Failed to restart measurement."); // ATtiny向け: デバッグ出力を削減
  }
  return raw_data;
}

// ---------------------------------------------------------------
// BMP280 補正計算関数 (データシート Section 8.2 32bit fixed point)
// ---------------------------------------------------------------

// 生の温度データを補正し、t_fineを計算する
// Returns temperature in 0.01 degree C.
// t_fine is set as a global variable and must be used for pressure compensation.
long bmp280_compensate_T_int32(long adc_T) {
  long var1, var2, T;
  var1 = ((((adc_T >> 3) - ((long)dig_T1 << 1))) * ((long)dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((long)dig_T1)) * ((adc_T >> 4) - ((long)dig_T1))) >> 12) *
          ((long)dig_T3)) >> 14;
  t_fine = var1 + var2; // t_fine はグローバル変数
  T = (t_fine * 5 + 128) >> 8;
  return T; // 戻り値は 0.01℃ 単位の温度 (整数)
}

// 生の気圧データを補正する
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format.
// Output value of "24674867" represents 24674867/256 = 96386.2 Pa
unsigned long bmp280_compensate_P_int32(long adc_P) {
  long long var1, var2, p; // 64bit変数を使用

  var1 = ((long long)t_fine) - 128000;
  var2 = var1 * var1 * ((long long)dig_P6);
  var2 = var2 + ((var1 * ((long long)dig_P5)) << 17);
  var2 = var2 + (((long long)dig_P4) << 35);
  var1 = ((var1 * var1 * ((long long)dig_P3)) >> 8) + ((var1 * ((long long)dig_P2)) << 12);
  var1 = (((((long long)1) << 47) + var1)) * ((long long)dig_P1) >> 33;

  if (var1 == 0) {
    return 0;
  }

  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((long long)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((long long)dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((long long)dig_P7) << 4);

  // 戻り値は Q24.8 形式 (Pa * 256) の unsigned long (整数)
  return (unsigned long)p;
}

void setup() {
  // Serial通信の初期化 (ボーレートをATtinyで一般的な9600に)
  Serial.begin(9600);

  // I2Cピンを初期化
  i2c_init_pins();

  // BMP280を初期化・設定
  if (!bmp280_init()) {
    //  Serial.println("Init Failed!"); // 初期化失敗
    while (true); // 停止
  }
}

void loop() {
  // 生の温度・圧力データを読み出す
  uint32_t* raw_data = bmp280_read_raw_data();
  uint32_t raw_pressure = raw_data[0];
  uint32_t raw_temperature = raw_data[1];

  // 生データが有効か確認
  if (raw_pressure == 0 && raw_temperature == 0) {
    // Serial.println("Read Error"); // ATtiny向け: デバッグ出力を削減
  } else {
    // 生データから補正された温度を計算 (0.01℃ 単位の整数)
    // raw_temperature はuint32_tですが、補正関数はlong (signed 32bit) を期待するのでキャスト
    int comp_temp_centi_c = bmp280_compensate_T_int32((long)raw_temperature);

    // 補正された気圧を計算 (Pa * 256 単位の unsigned long)
    // raw_pressure はuint32_tですが、補正関数はlong (signed 32bit) を期待するのでキャスト
    unsigned long comp_press_pa_div_256 = bmp280_compensate_P_int32((long)raw_pressure);
    // 気圧: Pa 単位の整数に変換して表示 (Pa * 256 を 256 で割る)
    unsigned long pressure_pa = comp_press_pa_div_256 / 256;

    // 結果を表示 (浮動小数点を使わない整数表示)
    // 温度: 0.01℃ 単位の整数そのまま表示 (例: 2508 -> 25.08℃)
    Serial.print("T=");
    Serial.println(comp_temp_centi_c); // 0.01℃単位
    //  Serial.print(" P=");
    //  Serial.println(pressure_pa); // Pa単位の整数

  }

  // Forced mode の測定時間 + スタンバイ時間より長く待つ
  delay(2000);
}
