#include "client.h"
#include <string.h>
#include <stdlib.h>

#define ESP01_RX_BUFF_SIZE         1024

#define WIFI_SSID                  "FBI Surveillance Van"     // Setear Red Wi-Fi
#define WIFI_PASSWORD              "bq99ec3c21" // Setear password

#define SERVER_URL      "192.168.0.37"
#define SERVER_PORT     80
#define NOMBRE_DISPOSITIVO     "CIAA"
#define MAC_DISPOSITIVO     "ab:cd:ef:12:34:56"

CONSOLE_PRINT_ENABLE
DEBUG_PRINT_ENABLE

uartMap_t uartEsp01 = UART_232;
uartMap_t uartDebug = UART_USB;

// ESP01 Rx Buffer
static char ESP_RESPONSE_BUFFER[ ESP01_RX_BUFF_SIZE ];
static uint32_t ESP_RESPONSE_BUFFER_SIZE = ESP01_RX_BUFF_SIZE;

static char TCP_DATA_TO_SEND[100];

static uint32_t UDP_PORT;

static void ESP_cleanRxBuffer( void );

static bool_t ESP_connecToWifiAP();

static bool_t ESP_connectToServer(bool_t tcp);

static bool_t ESP_sendData( char* strData, uint32_t strDataLen,bool_t udp );

static bool_t ESP_disconnectFromServer();

static char* HTTP_generatePostRequest();

static char* HTTP_generateDeleteRequest();

static char* HTTP_getBodyFromResponse(char* buffer);




static void ESP_cleanRxBuffer( void ){
   ESP_RESPONSE_BUFFER_SIZE = ESP01_RX_BUFF_SIZE;
   memset( ESP_RESPONSE_BUFFER, 0, ESP_RESPONSE_BUFFER_SIZE );
}

static char* HTTP_generatePostRequest(){
	TCP_DATA_TO_SEND[0] = 0;
	char body[70];
	body[0] = 0;
	char len[3];
	strcat( body, "{\"mac\":\"" );
	strcat( body, MAC_DISPOSITIVO );
	strcat( body, "\",\"nombre\":\"" );
	strcat( body, NOMBRE_DISPOSITIVO );
	strcat( body, "\"}" );
	itoa(strlen(body),len,10);
	strcat( TCP_DATA_TO_SEND, "POST /dispositivos HTTP/1.1\nContent-Type: application/json\nContent-Length: ");
	strcat( TCP_DATA_TO_SEND, len);
	strcat( TCP_DATA_TO_SEND, "\n\n");
	strcat( TCP_DATA_TO_SEND, body);
	strcat( TCP_DATA_TO_SEND, "\n");
	return TCP_DATA_TO_SEND;
}

static char* HTTP_generateDeleteRequest(){
	TCP_DATA_TO_SEND[0] = 0;
	char port[5] = "8081";
	itoa(UDP_PORT,port,10);
	strcat(TCP_DATA_TO_SEND,"DELETE /dispositivos/");
	strcat(TCP_DATA_TO_SEND, port);
	strcat(TCP_DATA_TO_SEND," HTTP/1.1\n\n");
	return TCP_DATA_TO_SEND;
}

static char* HTTP_getBodyFromResponse(char* buffer){
	const char needle[27] = "Connection: keep-alive\r\n\r\n";
	char* ptr = strstr(buffer,needle);
	if(ptr!=NULL){
		ptr+=25;
		return ptr;
	}
	debugPrintString( "ERROR al recuperar el puerto" );
	return NULL;
}

static uint32_t HTTP_getPuerto(char* response){
	char* body = HTTP_getBodyFromResponse(response);
	if(body==NULL){
		debugPrintlnString("body==NULL");
		return 0;
	}
	debugPrintString("[body] => ");
	debugPrintlnString(body);
	return atoi(body);
}

static bool_t ESP_sendData( char* strData, uint32_t strDataLen, bool_t tcp ){

   bool_t retVal = FALSE;

   ESP_cleanRxBuffer();

   // Envio datos TCP/IP al servidor.
   debugPrintlnString( ">>>> Envio datos al servidor..." );

   consolePrintString( "AT+CIPSEND=" );
   if(tcp){
	   consolePrintString("3,");
	   consolePrintInt( strDataLen + 2 );// El mas 2 es del \r\n
   }else{
	   consolePrintString("4,");
	   consolePrintInt( strDataLen );
   }
   consolePrintString( "\r\n" );
   retVal = receiveBytesUntilReceiveStringOrTimeoutBlocking(
	  uartEsp01,
	  "\r\n\r\nOK\r\n>", 9,
	  ESP_RESPONSE_BUFFER, &ESP_RESPONSE_BUFFER_SIZE,
	  5000
   );
   if(retVal){
	 consolePrintString( strData );
	  if(tcp){
		 ESP_cleanRxBuffer();
		 consolePrintString("\r\n");
		 retVal = receiveBytesUntilReceiveStringOrTimeoutBlocking(
			uartEsp01,
			NULL, 0,
			ESP_RESPONSE_BUFFER, &ESP_RESPONSE_BUFFER_SIZE,
			5000
		 );
		 debugPrintlnString(ESP_RESPONSE_BUFFER);
		 return TRUE;
	  }
   }else{
	   debugPrintString( "Error al realizar CIPSEND" );
	   debugPrintString( ESP_RESPONSE_BUFFER );
   }
   return retVal;
}


static bool_t ESP_connectToServer(bool_t tcp){

   bool_t retVal = FALSE;

   ESP_cleanRxBuffer();

   debugPrintlnString( ">>>> Conectando al servidor " );

   if(tcp){
	   consolePrintString( "AT+CIPSTART=3,\"TCP\",\"" );
   }else{
	   consolePrintString( "AT+CIPSTART=4,\"UDP\",\"" );
   }
   consolePrintString( SERVER_URL );
   consolePrintString( "\"," );
   if(tcp){
	   consolePrintInt( SERVER_PORT );
   }else{
	   consolePrintInt( UDP_PORT );
   }
   consolePrintString( "\r\n" );
   // No poner funciones entre el envio de comando y la espera de respuesta
   retVal = receiveBytesUntilReceiveStringOrTimeoutBlocking(
               uartEsp01,
               NULL, 0,
               ESP_RESPONSE_BUFFER, &ESP_RESPONSE_BUFFER_SIZE,
               10000
            );
   const char needle[8] = "CONNECT";
   char* ptr = strstr(ESP_RESPONSE_BUFFER,needle);
   retVal = ptr!=NULL;

   if( !retVal ){
	  debugPrintlnString( ">>>>    Error: No se puede conectar al servidor: " );
	  debugPrintlnString( ESP_RESPONSE_BUFFER );
   }
   return retVal;
}

static bool_t ESP_disconnectFromServer(){

   bool_t retVal = FALSE;

   ESP_cleanRxBuffer();

   debugPrintString( ">>>> Desconectando del servidor" );
   consolePrintString( "AT+CIPCLOSE=5 \r\n" );

   // No poner funciones entre el envio de comando y la espera de respuesta
   retVal = receiveBytesUntilReceiveStringOrTimeoutBlocking(
               uartEsp01,
               "OK\r\n", 4,
               ESP_RESPONSE_BUFFER, &ESP_RESPONSE_BUFFER_SIZE,
               10000
            );
   if( !retVal ){
      debugPrintString( ">>>>    Error: No se puede desconectar del servidor:" );
	  debugPrintString( ESP_RESPONSE_BUFFER );
   }
   return retVal;
}


static bool_t ESP_connecToWifiAP(){

   bool_t retVal = FALSE;

   ESP_cleanRxBuffer();

   debugPrintlnString( ">>>> Conectando a la red Wi-Fi: " );
   consolePrintString( "AT+CWJAP=\"" );
   consolePrintString( WIFI_SSID );
   consolePrintString( "\",\"" );
   consolePrintString( WIFI_PASSWORD );
   consolePrintString( "\"\r\n" );

   // No poner funciones entre el envio de comando y la espera de respuesta
   retVal = receiveBytesUntilReceiveStringOrTimeoutBlocking(
               uartEsp01,
               "WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n", 35,
               ESP_RESPONSE_BUFFER, &ESP_RESPONSE_BUFFER_SIZE,
               10000
            );
   if( retVal ){
	  debugPrintlnString(">>>> Conectado a la red" );
   } else{
      debugPrintlnString( ">>>>    Error: No se puede conectar a la red: " );
      debugPrintlnString( ESP_RESPONSE_BUFFER );
   }
   return retVal;
}


bool_t CLIENT_init( uartMap_t uartForEsp, uartMap_t uartForDebug, uint32_t baudRate ){

   bool_t retVal = FALSE;
   uartEsp01 = uartForEsp;
   uartDebug = uartForDebug;

   // Inicializar UART_USB como salida de debug
   debugPrintConfigUart( uartDebug, baudRate );
   debugPrintlnString( ">>>> UART_USB configurada como salida de debug." );

   // Inicializr otra UART donde se conecta el ESP01 como salida de consola
   consolePrintConfigUart( uartEsp01, baudRate );
   debugPrintlnString( ">>>> UART_ESP (donde se conecta el ESP01), \r\n>>>> configurada como salida de consola.\r\n" );

   consolePrintString( "AT\r\n" );
   retVal = waitForReceiveStringOrTimeoutBlocking( uartEsp01, "AT\r\n", 4, 500 );
   if( retVal ){
      debugPrintlnString( ">>>>    Modulo ESP01 Wi-Fi detectado.\r\n" );
      consolePrintString("AT+CIPMUX=1\r\n");
      retVal = waitForReceiveStringOrTimeoutBlocking(uartEsp01,"OK\r\n",4,500);
   } else{
      debugPrintlnString( ">>>>    Error: Modulo ESP01 Wi-Fi No detectado!!\r\n" );
   }
   return ESP_connecToWifiAP();
}


bool_t CLIENT_register(){

	char* request;
	uint32_t puerto;
	if(!ESP_connectToServer(TRUE))
	  return FALSE;

	request = HTTP_generatePostRequest();

	 if(!ESP_sendData( request, strlen(request) ,TRUE) )
		  return FALSE;
	 puerto = HTTP_getPuerto(ESP_RESPONSE_BUFFER);
	 if(!puerto){
		 debugPrintlnString( ">>>> FALLO AL RECIBIR EL PUERTO" );
		 return FALSE;
	 }
	 debugPrintString( ">>>> PUERTO RECIBIDO DEL SERVIDOR: " );
	 debugPrintlnInt(puerto);

	 UDP_PORT = puerto;

	 return ESP_connectToServer(FALSE);

}

bool_t CLIENT_unregister(){
	char* request;
	if(!ESP_connectToServer(TRUE))
	  return FALSE;

	request = HTTP_generateDeleteRequest();
	if(!ESP_sendData( request, strlen(request),TRUE)){
		return FALSE;
	}
	return ESP_disconnectFromServer();

}

bool_t CLIENT_send(char* buffer, uint32_t size){
	return ESP_sendData(buffer,size,FALSE);
}


