#ifndef GPE_CONF_DEVICE_H
#define GPE_CONF_DEVICE_H

typedef enum
{
	DEV_UNKNOWN = 0,
	DEV_IPAQ_SA,
	DEV_IPAQ_PXA,
	DEV_SIMPAD,
	DEV_RAMSES,
	DEV_ZAURUS_COLLIE,
	DEV_ZAURUS_POODLE,
	DEV_ZAURUS_SHEPHERED,
	DEV_ZAURUS_HUSKY,
	DEV_ZAURUS_AKITA,
	DEV_ZAURUS_SPITZ,
	DEV_ZAURUS_CORGI,
	DEV_ZAURUS_BORZOI,
	DEV_NOKIA_770,
	DEV_NETBOOK_PRO,
	DEV_HW_INTEGRAL,
	DEV_CELLON_8000,
	DEV_JOURNADA,
	DEV_SGI_O2,
	DEV_SGI_INDY,
	DEV_SGI_INDIGO2,
	DEV_SGI_OCTANE,
	DEV_X86,
	DEV_POWERPC,
	DEV_SPARC,
	DEV_ALPHA,
	DEV_HTC_UNIVERSAL,
	DEV_ETEN_G500,
	DEV_HW_SKEYEPADXSL,
	DEV_N800,
	DEV_AXIM_X5X,
	DEV_PALM_TX,
	DEV_GTA01,
	DEV_NEON,
	DEV_MAINSTONE,
	DEV_N810,
	DEV_MAX
} DeviceID_t;

typedef enum {
	DEVICE_FEATURE_NONE        = 0x00,
	DEVICE_FEATURE_PDA         = 0x01,
	DEVICE_FEATURE_PC          = 0x02,
	DEVICE_FEATURE_TABLET      = 0x04,
	DEVICE_FEATURE_CELLPHONE   = 0x08,
	DEVICE_FEATURE_MDE         = 0x10
} DeviceFeatureID_t;

DeviceID_t device_get_id (void);
DeviceFeatureID_t device_get_features (void);
const gchar *device_get_name (void);
const gchar *device_get_manufacturer (void);
const gchar *device_get_type (void);
gchar *device_get_specific_file (const gchar *basename);


#endif
