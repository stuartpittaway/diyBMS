#ifndef softap_H_
#define softap_H_


void setupAccessPoint(void);

void SetupManagementRedirect(void);

void HandleWifiClient(void);

extern int cell_array_index;
extern int cell_array_max;
extern cell_module cell_array[24];

extern bool runProvisioning;


#endif

