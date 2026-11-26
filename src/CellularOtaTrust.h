#ifndef NUVIOT_CELLULAR_OTA_TRUST_H
#define NUVIOT_CELLULAR_OTA_TRUST_H

// Software Logistics owns this trust anchor. Populate it only after the
// approved DEV endpoint is online and its actual certificate chain has been
// inspected. Never place a leaf certificate or private key here.
//
// The empty default deliberately makes cellular HTTPS fail closed while the
// endpoint chain is pending. This is a build-time library constant, not a
// customer-configurable trust store.
#ifndef NUVIOT_CELLULAR_OTA_TRUST_ANCHOR_PEM
#define NUVIOT_CELLULAR_OTA_TRUST_ANCHOR_PEM ""
#endif

static const char CellularOtaTrustAnchorPem[] = NUVIOT_CELLULAR_OTA_TRUST_ANCHOR_PEM;
static const char CellularOtaTrustAnchorFile[] = "nuviot-dfu-root.cer";

#endif
