#include "pdu.h"

size_t calc_pdu_size(struct PDU pdu)
{
  size_t body_size;
  switch (pdu.type)
  {
  case PDU_CONTENT_REGISTRATION:
    body_size = sizeof(struct PDUContentRegistrationBody);
    break;
  case PDU_CONTENT_DOWNLOAD_REQUEST:
    body_size = sizeof(struct PDUContentDownloadRequestBody);
    break;
  case PDU_CONTENT_AND_SERVER_SEARCH:
    body_size = sizeof(struct PDUContentDownloadRequestBody);
  case PDU_CONTENT_DEREGISTRATION:
    body_size = sizeof(struct PDUContentRegistrationBody);
    break;
  case PDU_CONTENT_DATA:
    body_size = sizeof(struct PDUContentDataBody);
    break;
  case PDU_ONLINE_CONTENT_LIST:
    body_size = sizeof(struct PDUContentListingBody);
    break;
  case PDU_ACKNOWLEDGEMENT:
    body_size = sizeof(struct PDUAcknowledgement);
    break;
  case PDU_ERROR:
    body_size = sizeof(struct PDUErrorBody);
    break;
  default:
    body_size = 0;
    break;
  }

  return sizeof(pdu.type) + body_size;
}