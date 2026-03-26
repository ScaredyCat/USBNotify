#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE)
#define CFG_TUD_ENABLED           1
#define CFG_TUD_HID               1  // This fixes the 'hid_report_type_t' error
#define CFG_TUD_HID_EP_BUFSIZE    64

#ifdef __cplusplus
 }
#endif

#endif

