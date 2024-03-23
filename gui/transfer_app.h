#ifndef KSP_TRANSFER_APP_H
#define KSP_TRANSFER_APP_H

#include "ephem.h"

enum LastTransferType {TF_FLYBY, TF_CAPTURE, TF_CIRC};

void start_transfer_app();


struct Ephem ** get_body_ephems();

#endif //KSP_TRANSFER_APP_H
