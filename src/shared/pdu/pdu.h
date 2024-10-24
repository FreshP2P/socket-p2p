#ifndef _P2P_PDU
#define _P2P_PDU

#define STANDARD_BODY_SIZE 100
#define DATA_BODY_SIZE 1024

enum PDUType
{
  PDU_CONTENT_REGISTRATION = 'R',
  PDU_CONTENT_DOWNLOAD_REQUEST = 'D',
  PDU_CONTENT_AND_SERVER_SEARCH = 'S',
  PDU_CONTENT_DEREGISTRATION = 'T',
  PDU_CONTENT_DATA = 'D',
  PDU_ONLINE_CONTENT_LIST = 'O',
  PDU_ACKNOWLEDGEMENT = 'A',
  PDU_ERROR = 'E'
};

struct PDU
{
  enum PDUType type;
  
  union
  {
    // TODO: As we develop the app, we will add the proper
    // contents per PDU type. Note that we should always check the type
    // before assuming the field that represents the body is accessed.
    // Some examples are shown below.
    
    // D: Bytes of parts of a file
    char data[DATA_BODY_SIZE];
    
    // E: Message of the error
    char error_message[STANDARD_BODY_SIZE];
  } body;
};
#endif