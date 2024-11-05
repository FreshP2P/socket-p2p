#ifndef _P2P_PDU
#define _P2P_PDU

#include <netinet/in.h>
#include "../config/constants.h"

enum PDUType
{
  PDU_CONTENT_REGISTRATION = 'R',
  PDU_CONTENT_DOWNLOAD_REQUEST = 'D',
  PDU_CONTENT_AND_SERVER_SEARCH = 'S',
  PDU_CONTENT_DEREGISTRATION = 'T',
  PDU_CONTENT_DATA = 'C',
  PDU_ONLINE_CONTENT_LIST = 'O',
  PDU_ACKNOWLEDGEMENT = 'A',
  PDU_ERROR = 'E'
};

typedef struct PeerContentInfo
{
  char peer_name[PEER_NAME_SIZE + 1];
  char content_name[CONTENT_NAME_SIZE + 1];
} peer_content_info_t;

struct PDUContentRegistrationBody
{
  peer_content_info_t info;
  struct sockaddr_in address;
};

struct PDUContentDownloadRequestBody
{
  peer_content_info_t info;
  
  // address as response from the index server
  // clients can supply this as empty
  struct sockaddr_in address;
};

struct PDUContentDeregistrationBody
{
  peer_content_info_t info;
};

struct PDUContentDataBody
{
  size_t data_len;
  char data[DATA_BODY_SIZE];
};

struct PDUContentListingBody
{
  int end_of_list; // 0: false, 1: true
  peer_content_info_t registered_content;
};

struct PDUAcknowledgement
{
  char peer_name[PEER_NAME_SIZE + 1];
};

struct PDUErrorBody
{
  char message[STANDARD_BODY_SIZE + 1];
};

union PDUBody
{
  // R type body
  struct PDUContentRegistrationBody content_registration;
  
  // D/S type body
  struct PDUContentDownloadRequestBody content_download_req;

  // T type body
  struct PDUContentDeregistrationBody content_deregistration;

  // C type body
  struct PDUContentDataBody content_data;
  
  // O type body
  struct PDUContentListingBody content_listing;

  // A type body
  struct PDUAcknowledgement ack;

  // E type body
  struct PDUErrorBody error;
};

struct PDU
{
  enum PDUType type;

  // TODO: As we develop the app, we will add the proper
  // contents per PDU type. Note that we should always check the type
  // before assuming the field that represents the body is accessed.
  // Some examples are shown below.
  union PDUBody body;
};

size_t calc_pdu_size(struct PDU pdu);

#endif