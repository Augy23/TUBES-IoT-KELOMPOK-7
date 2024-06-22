#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// RTC
RTC_DS3231 rtc;

char dataHari[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
String hari;
int tanggal, bulan, tahun, jam, menit, detik;
int jam1, menit1, jam2, menit2;
int jamMakan, menitMakan;
int makan, b, c;
float suhu;

// Token API dan chat ID Telegram
const char* botToken = "7410634346:AAGW0-5OUuI0f7KVdGiaXqB9q9NwMvOfnmY";
const String chat_id = "6928141045";

// WiFi credentials
const char* ssid = "AGNOLOGIA";
const char* password = "BAYARDULU";

// Bot Telegram
WiFiClientSecure client;
UniversalTelegramBot myBot(botToken, client);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo
Servo mekanik;

// Buzzer
#define BUZZER 14

// Variables for parameters
String arrData[4];

void setup() {
  Serial.begin(9600);

  // Setup WiFi
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nTerhubung ke WiFi");

  // Setup Telegram Bot
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("Mencoba menghubungkan ke bot Telegram...");
  if (myBot.getMe()) {
    Serial.println("Bot Telegram terhubung");
  } else {
    Serial.println("Koneksi ke bot Telegram gagal");
  }

  // Setup EEPROM
  EEPROM.begin(512);
  jam1       = EEPROM.read(0);
  menit1     = EEPROM.read(1);
  jam2       = EEPROM.read(2);
  menit2     = EEPROM.read(3);
  jamMakan   = EEPROM.read(4);
  menitMakan = EEPROM.read(5);
  delay(100);

  // Setup LCD
  lcd.init();
  lcd.backlight();

  // Setup Buzzer
  pinMode(BUZZER, OUTPUT);

  // Setup Servo
  mekanik.attach(18);
  mekanik.write(0);
  Serial.println("Servo terhubung pada pin 18");

  // Setup RTC
  if (!rtc.begin()) {
    Serial.println("RTC tidak ditemukan");
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC kehilangan daya, mengatur ulang waktu!");
    // Mengatur waktu RTC ke waktu kompilasi sketsa
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();
  hari    = dataHari[now.dayOfTheWeek()];
  tanggal = now.day();
  bulan   = now.month();
  tahun   = now.year();
  jam     = now.hour();
  menit   = now.minute();
  detik   = now.second();
  suhu    = rtc.getTemperature();

  // Debug waktu dan tanggal
  Serial.print(hari);
  Serial.print(", ");
  Serial.print(tanggal);
  Serial.print("-");
  Serial.print(bulan);
  Serial.print("-");
  Serial.print(tahun);
  Serial.print(" ");
  Serial.print(jam);
  Serial.print(":");
  Serial.print(menit);
  Serial.print(":");
  Serial.println(detik);

  // Tampilkan di LCD, hanya jika berubah
  lcd.setCursor(0, 0);
  lcd.print(String() + hari + "," + tanggal + "-" + bulan + "-" + tahun);
  lcd.setCursor(0, 1);
  lcd.print(String() + (jam < 10 ? "0" : "") + jam + ":" + (menit < 10 ? "0" : "") + menit + ":" + (detik < 10 ? "0" : "") + detik);
  delay(10); // Kurangi delay untuk meningkatkan kecepatan pembaruan

  // Baca pesan dari Telegram
  int numNewMessages = myBot.getUpdates(myBot.last_message_received + 1);
  if (numNewMessages > 0) {
    Serial.print("Jumlah pesan baru: ");
    Serial.println(numNewMessages);
  }

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id_received = String(myBot.messages[i].chat_id);
    String text = myBot.messages[i].text;
    Serial.println("Pesan masuk:");
    Serial.print("Chat ID: ");
    Serial.println(chat_id_received);
    Serial.print("Teks: ");
    Serial.println(text);

    // Penanganan perintah
    if (text.equals("menu") || text.equals("/menu")) {
      myBot.sendMessage(chat_id, "Kirim:\n1 = Beri makan sekarang\n2 = Setel waktu pemberian makan\n3 = Lihat waktu pemberian makan terakhir\n4 = Waktu pemberian makan terakhir");
      Serial.println("Menanggapi perintah 'menu'");
    } else if (text.equals("1")) {
      makan = 1;
      Serial.println("Perintah makan diterima");
    } else if (text.equals("2")) {
      myBot.sendMessage(chat_id, "Silakan masukkan jam & menit\nFormat: Jam1#Menit1#Jam2#Menit2");
    } else if (text.equals("3")) {
      myBot.sendMessage(chat_id, String() + "Waktu pemberian makan1 = " + jam1 + ":" + menit1 + "\nWaktu pemberian makan2 = " + jam2 + ":" + menit2);
    } else if (text.equals("4")) {
      myBot.sendMessage(chat_id, String() + "Waktu pemberian makan terakhir = " + jamMakan + ":" + menitMakan);
    } else {
      // Parsing pesan berdasarkan delimiter #
      int index = 0;
      for (int j = 0; j < 4; j++) {
        arrData[j] = "";
      }

      for (int j = 0; j < text.length(); j++) {
        char delimiter = '#';
        if (text[j] != delimiter) {
          if (index < 4) {
            arrData[index] += text[j];
          }
        } else {
          index++;
        }
      }

      if (index == 3) { // Pastikan kita mendapatkan 4 data
        jam1 = arrData[0].toInt();
        menit1 = arrData[1].toInt();
        jam2 = arrData[2].toInt();
        menit2 = arrData[3].toInt();

        EEPROM.write(0, jam1);
        EEPROM.write(1, menit1);
        EEPROM.write(2, jam2);
        EEPROM.write(3, menit2);
        EEPROM.commit();

        myBot.sendMessage(chat_id, "Waktu pemberian makan berhasil diatur");
        Serial.println("Waktu pemberian makan berhasil diatur");
      } else {
        myBot.sendMessage(chat_id, "Format tidak valid. Harap masukkan format: Jam1#Menit1#Jam2#Menit2");
        Serial.println("Format tidak valid");
      }
    }
  }

  // Periksa apakah saatnya memberi makan
  if (((jam == jam1 && menit == menit1) || (jam == jam2 && menit == menit2)) && (menit != menitMakan)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Memberi makan");
    kasih_pakan(5);
    lcd.setCursor(0, 1);
    lcd.print("Otomatis");
    delay(3000);
    lcd.clear();
    jamMakan = jam; menitMakan = menit;
    EEPROM.write(4, jamMakan);
    EEPROM.write(5, menitMakan);
    EEPROM.commit();
    makan = 0;
  } else if (makan == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Memberi makan");
    kasih_pakan(5);
    lcd.setCursor(0, 1);
    lcd.print("Manual");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Pemberian pakan");
    lcd.setCursor(0, 1);
    lcd.print("selesai");
    delay(3000);
    lcd.clear();
    jamMakan = jam; menitMakan = menit;
    EEPROM.write(4, jamMakan);
    EEPROM.write(5, menitMakan);
    EEPROM.commit();
    makan = 0;
    b = 0;
    c = 0;
  }
}

// Fungsi untuk mengontrol servo untuk pemberian makan
void kasih_pakan(int pakan) {
  for (int i = 0; i < pakan; i++) {
    mekanik.write(180);
    Serial.println("Servo pada 180 derajat");
    digitalWrite(BUZZER, HIGH); // Nyalakan buzzer
    delay(500);
    mekanik.write(0);
    Serial.println("Servo pada 0 derajat");
    digitalWrite(BUZZER, LOW); // Matikan buzzer
    delay(500);
  }
}
