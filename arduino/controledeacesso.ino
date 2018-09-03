#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PubSubClient.h> 
#include <FS.h> 


#define SS_PIN 4  //D2
#define RST_PIN 5 //D1


const char* ssid = "Freire";
const char* password = "F3rr31r@Fr31r3";
const char* MQTTusr = "admin";
const char* MQTTpass = "relogio.";
String mac;
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
int statuss = 0;
bool out = false;

// MQTT
const char* BROKER_MQTT = "192.168.11.09"; //URL do broker MQTT que se deseja
utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto
espClient

void conectaMQTT() 
{
    while (!MQTT.connected()) 
    {
        char ID_MQTT[20];
        mac.toCharArray(ID_MQTT,mac.length()+1);
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT,MQTTusr,MQTTpass)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe("porta"); 
        } 
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
 
void setup() {
  Serial.begin(115200);
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  Serial.println("Iniciando...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  mac=WiFi.macAddress();
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an
output
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Conexao falhou! Reiniciando...");
    delay(5000);
    ESP.restart();
  }

    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta
deve ser conectado
    MQTT.setCallback(callback);            //atribui função de callback 
    conectaMQTT();
  // Porta padrao do ESP8266 para OTA eh 8266 - Voce pode mudar ser quiser, mas
  // deixe indicado!
  // ArduinoOTA.setPort(8266);
 
  // O Hostname padrao eh esp8266-[ChipID], mas voce pode mudar com essa funcao
  // ArduinoOTA.setHostname("nome_do_meu_esp8266");
 
  // Nenhuma senha eh pedida, mas voce pode dar mais seguranca pedindo uma
  // senha pra gravar
  // ArduinoOTA.setPassword((const char *)"123");
 
  ArduinoOTA.onStart([]() {
    Serial.println("Inicio...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("nFim!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progresso: %u%%r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Autenticacao Falhou");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no Inicio");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na Conexao");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha na Recepcao");
    else if (error == OTA_END_ERROR) Serial.println("Falha no Fim");
  });
  ArduinoOTA.begin();
  Serial.println("Pronto");
  Serial.print("Endereco IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Endereco MAC: ");
  Serial.println(WiFi.macAddress());


}

void AbrePorta() {

  if(out){
    digitalWrite(LED_BUILTIN, HIGH );  //liga
    out=false;                             
   }
   else{
    digitalWrite(LED_BUILTIN, LOW );  //desliga
    out=true;
   }
}
bool TemCartao(){
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return false;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return false;
  }
  return true;
}

String RetornaUid()
{
    //Show UID on serial monitor
  Serial.println();
  Serial.print(" UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  Serial.println();
  return content.substring(1);
}

void createFile(String Nome){
  File wFile;
 
  //Cria o arquivo se ele não existir
  if(SPIFFS.exists(Nome)){
    Serial.println("Arquivo ja existe!");
  } else {
    Serial.println("Criando o arquivo...");
    wFile = SPIFFS.open(Nome,"w+");
 
    //Verifica a criação do arquivo
    if(!wFile){
      Serial.println("Erro ao criar arquivo "+Nome);
    } else {
      Serial.println("Arquivo criado com sucesso - "+Nome);
    }
  }
  wFile.close();
}

void deleteFile(String Nome) {
  //Remove o arquivo
  if(SPIFFS.remove(Nome)){
    Serial.println("Erro ao remover arquivo "+Nome);
  } else {
    Serial.println("Arquivo removido com sucesso - "+Nome);
  }
}

bool VerificaAcesso(String cartaoUID) { return SPIFFS.exists(cartaoUID); }

void MQTTEnvia(String cartaoUID)
{
    String msg;
    msg=cartaoUID+","+mac;
    char payload[50];
    msg.toCharArray(payload,msg.length()+1);
    MQTT.publish("cartao", payload);
    Serial.println(payload);
    Serial.print(" enviado ao broker!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String Dados[3];
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  int j=0;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    if((char)payload[i]==',') j++;
    else Dados[j]+=(char)payload[i];
  }
  Serial.println();
  RecebeDados(Dados);
  
}

void RecebeDados(String Dados[])
{
    Serial.print("Cartao: ");  
    Serial.println(Dados[0]);
    Serial.print("MAC: ");
    Serial.println(Dados[1]);
    Serial.print("Permissao: ");
    Serial.println(Dados[2]);

}
void loop() {
  bool cartaoLido=false;
  String cartaoUID;
  
  ArduinoOTA.handle();
  MQTT.loop();
  if(TemCartao())
  {
         cartaoLido=true;
         cartaoUID=RetornaUid();
         MQTTEnvia(cartaoUID);
         delay(500);
  }
  if(cartaoLido)
  {
    if(VerificaAcesso(cartaoUID))
    { 
      AbrePorta();   
    }
  } 
}
