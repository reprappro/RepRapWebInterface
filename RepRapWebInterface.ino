/*

RepRap Web Interface

Adrian Bowyer

12 December 2012
 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,9);

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

#define MAX_FILES 5

#define null 0

File files[MAX_FILES];
bool inUse[MAX_FILES];

char line[1000];
char page[1000];
int lp;

void setup()
{
  lp = 0;
  for(int i=0; i < MAX_FILES; i++)
    inUse[i] = false;
    
  Serial.begin(9600);

  pinMode(10, OUTPUT);
  digitalWrite(10,LOW);   

  Ethernet.begin(mac, ip);
  server.begin();
  
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
  digitalWrite(10,HIGH);
 
  if (!SD.begin(4)) 
     Serial.println("SD initialization failed.");
    
  digitalWrite(10,LOW); 
     
  Serial.print("server is (still?) at ");
  Serial.println(Ethernet.localIP());    
     
}

void error(char* s)
{
  Serial.println(s); 
}

void comment(char* s)
{
  Serial.println(s); 
}

// Open a local file (for example on an SD card).

int OpenFile(char* fileName, bool write)
{
  int result = -1;
  for(int i=0; i < MAX_FILES; i++)
    if(!inUse[i])
    {
      result = i;
      break;
    }
  if(result < 0)
  {
      error("Max open file count exceeded.");
      return -1;    
  }
  
  if(!SD.exists(fileName))
  {
    if(!write)
    {
      error("File not found for reading");
      return -1;
    }
    files[result] = SD.open(fileName, FILE_WRITE);
  } else
  {
    if(write)
      files[result] = SD.open(fileName, FILE_WRITE);
    else
      files[result] = SD.open(fileName, FILE_READ);
  }

  inUse[result] = true;
  return result;
}

void Close(int file)
{
    files[file].close();
    inUse[file] = false;
}

bool Read(int file, unsigned char* b)
{
  if(!files[file].available())
    return false;
  *b = (unsigned char) files[file].read();
  return true;
}


void parseLine()
{
  if(!(line[0] == 'G' && line[1] == 'E' && line[2] == 'T'))
    return;
  int i = 5;
  int j = 0;
  while(line[i] != ' ')
  {
    page[j] = line[i];
    j++;
    i++;
  }
  page[j] = 0;
  if(!page[0])
    strcpy(page, "index.htm");
}


void loop() 
{
  int htmlFile;
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) 
  {
    comment("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) 
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
      
          htmlFile = OpenFile(page,false);
          unsigned char b;
          while(Read(htmlFile, &b))
            client.write(b);
          Close(htmlFile);
          
          page[0] = 0;
          lp = 0;
          break;
        }
        if (c == '\n') 
        {
          line[lp] = 0;
          parseLine();
          // you're starting a new line
          currentLineIsBlank = true;
          lp = 0;
        } else if (c != '\r') 
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
          line[lp]=c;
          lp++;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
}

