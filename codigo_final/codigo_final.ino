#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>

SoftwareSerial ESP8266(3, 2);  // RX | TX

String cadena;
String token_session;
String camara;

void setup() {
  Serial.begin(9600);
  ESP8266.begin(9600);
  configuracion_inicial();
  //login();
  //Serial.println(token_session);
  captura_img();
  Serial.println(camara);
}

void loop() {
}

void captura_img() {
  ESP8266.println("AT+CIPSTART=\"TCP\",\"192.168.100.155\",8080");
  if (ESP8266.find("OK")) {
    Serial.println("ESP8266 conectado con el servidor...");
    //Armamos el encabezado de la peticion http
    String peticionHTTP = "GET /photo.jpg HTTP/1.1\r\n";
    peticionHTTP = peticionHTTP + "Host: 192.168.100.155\r\n";
    peticionHTTP = peticionHTTP + "\r\n";
    //Enviamos el tamaño en caracteres de la peticion http:
    ESP8266.print("AT+CIPSEND=");
    ESP8266.println(peticionHTTP.length());

    if (ESP8266.find(">"))  // ">" indica que podemos enviar la peticion http
    {
      Serial.println("Enviando HTTP . . .");
      ESP8266.println(peticionHTTP);
      ESP8266.print((char)26);
      if (ESP8266.find("SEND OK")) {
        Serial.println("Peticion HTTP enviada:");
        Serial.println(peticionHTTP);
        Serial.println("Esperando respuesta...");
        boolean fin_respuesta = false;
        long tiempo_inicio = millis();
        camara = "";
        while (fin_respuesta == false) {
          while (ESP8266.available() > 0) {
            char c = ESP8266.read();
            Serial.write(c);
            camara.concat(c);  //guardamos la respuesta
          }
          //finalizamos si la respuesta es mayor a 1000 caracteres
          if (camara.length() > 1000)  //Pueden aumentar si tenen suficiente espacio en la memoria
          {
            Serial.println("La respuesta a excedido el tamaño maximo");
            ESP8266.println("AT+CIPCLOSE");
            if (ESP8266.find("OK"))
              Serial.println("Conexion finalizada");
            fin_respuesta = true;
          }
          if ((millis() - tiempo_inicio) > 35000)  //Finalizamos si ya han transcurrido 35 seg
          {
            Serial.println("Tiempo de espera agotado");
            ESP8266.println("AT+CIPCLOSE");
            if (ESP8266.find("OK"))
              Serial.println("Conexion finalizada");
            fin_respuesta = true;
          }
          if (camara.indexOf("CLOSED") > 0)  //si recibimos un CLOSED significa que ha finalizado la respuesta
          {
            Serial.println("Cadena recibida correctamente, conexion finalizada");
            fin_respuesta = true;
          }
        }
      } else {
        Serial.println("No se ha podido enviar HTTP.....");
      }
    }
  } else {
    Serial.println("No se ha podido conectarse con el servidor");
  }
}

void login() {
  ESP8266.println("AT+CIPSTART=\"TCP\",\"192.168.0.101\",8080");
  if (ESP8266.find("OK")) {
    Serial.println("ESP8266 conectado con el servidor...");
    //Armamos el encabezado de la peticion http
    String peticionHTTP = "POST /sesion/login HTTP/1.1\r\nHost: 192.168.0.101\r\nConnection: close\r\ncorreo:im29@uteq.edu.ec\r\nclave:12345\r\n\r\n\r\n\r\n";
    //Enviamos el tamaño en caracteres de la peticion http:
    ESP8266.print("AT+CIPSEND=");
    ESP8266.println(peticionHTTP.length());
    if (ESP8266.find(">"))  // ">" indica que podemos enviar la peticion http
    {
      Serial.println("Enviando HTTP . . .");
      ESP8266.println(peticionHTTP);
      if (ESP8266.find("SEND OK")) {
        Serial.println("Peticion HTTP enviada:");
        Serial.println(peticionHTTP);
        Serial.println("Esperando respuesta...");
        boolean fin_respuesta = false;
        long tiempo_inicio = millis();
        token_session = "";
        while (fin_respuesta == false) {
          while (ESP8266.available() > 0) {
            char c = ESP8266.read();
            Serial.write(c);
            token_session.concat(c);  //guardamos la respuesta
          }
          //finalizamos si la respuesta es mayor a 500 caracteres
          if (token_session.length() > 500)  //Pueden aumentar si tenen suficiente espacio en la memoria
          {
            Serial.println("La respuesta a excedido el tamaño maximo");

            ESP8266.println("AT+CIPCLOSE");
            if (ESP8266.find("OK"))
              Serial.println("Conexion finalizada");
            fin_respuesta = true;
          }
          if ((millis() - tiempo_inicio) > 10000)  //Finalizamos si ya han transcurrido 10 seg
          {
            Serial.println("Tiempo de espera agotado");
            ESP8266.println("AT+CIPCLOSE");
            if (ESP8266.find("OK"))
              Serial.println("Conexion finalizada");
            fin_respuesta = true;
          }
          if (token_session.indexOf("CLOSED") > 0)  //si recibimos un CLOSED significa que ha finalizado la respuesta
          {
            Serial.println("Cadena recibida correctamente, conexion finalizada");
            fin_respuesta = true;
          }
        }
      } else {
        Serial.println("No se ha podido enviar HTTP.....");
      }
    }
  } else {
    Serial.println("No se ha podido conectarse con el servidor");
  }
}

void configuracion_inicial() {
  ESP8266.println("AT");
  if (ESP8266.find("OK"))
    Serial.println("Respuesta AT correcto");
  else
    Serial.println("Error en ESP8266");
  ESP8266.println("AT+CWMODE=3");
  if (ESP8266.find("OK"))
    Serial.println("ESP8266 en DOBLE modo");
  //Nos conectamos a una red wifi
  ESP8266.println("AT+CWJAP=\"INTERCOM FAMILIA MOREIRA\",\"Darlyta2022\"");
  Serial.println("Conectandose a la red ...");
  ESP8266.setTimeout(10000);
  if (ESP8266.find("OK"))
    Serial.println("WIFI conectado");
  else
    Serial.println("Error al conectarse en la red");
  ESP8266.setTimeout(2000);
}