/*
 * Copyright 2011, Haiku Inc. All Rights Reserved.
 * Copyright 2009, Ithamar Adema, <ithamar.adema@team-embedded.nl>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _USB_VIDEO_H
#define _USB_VIDEO_H


#define VC_CONTROL_UNDEFINED		0x0
#define VC_VIDEO_POWER_MODE_CONTROL	0x1
#define VC_REQUEST_ERROR_CODE_CONTROL	0x2

typedef struct usbvc_class_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;	
} _PACKED usbvc_class_descriptor;

struct usbvc_input_header_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	numFormats;
	uint16	totalLength;
	uint8	endpointAddress;
	uint8	info;
	uint8	terminalLink;
	uint8	stillCaptureMethod;
	uint8	triggerSupport;
	uint8	triggerUsage;
	uint8	controlSize;
	uint8	controls[0];
} _PACKED;

struct usbvc_output_header_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	numFormats;
	uint16	totalLength;
	uint8	endpointAddress;
	uint8	terminalLink;
	uint8	controlSize;
	uint8	controls[0];
} _PACKED;

typedef uint8 usbvc_guid[16];

struct usbvc_format_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	formatIndex;
	uint8	numFrameDescriptors;
	union {
		struct {
			usbvc_guid format;
			uint8	bytesPerPixel;
			uint8	defaultFrameIndex;
			uint8	aspectRatioX;
			uint8	aspectRatioY;
			uint8	interlaceFlags;
			uint8	copyProtect;
		} uncompressed;
		struct {
			uint8	flags;
			uint8	defaultFrameIndex;
			uint8	aspectRatioX;
			uint8	aspectRatioY;
			uint8	interlaceFlags;
			uint8	copyProtect;
		} mjpeg;
	};
} _PACKED;

struct usbvc_frame_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	frameIndex;
	uint8	capabilities;
	uint16	width;
	uint16	height;
	uint32	minBitRate;
	uint32	maxBitRate;
	uint32	maxVideoFrameBufferSize;
	uint32	defaultFrameInterval;
	uint8	frameIntervalType;
	union {
		struct {
			uint32	minFrameInterval;
			uint32	maxFrameInterval;
			uint32	frameIntervalStep;
		} continuous;
		uint32	discreteFrameIntervals[0];
	};
} _PACKED;

typedef struct {
	uint16	width;
	uint16	height;
} _PACKED usbvc_image_size_pattern;

struct usbvc_still_image_frame_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	endpointAddress;
	uint8	numImageSizePatterns;
	usbvc_image_size_pattern imageSizePatterns[0];
	uint8 NumCompressionPatterns() const { return *(CompressionPatterns() - 1); }
	const uint8* CompressionPatterns() const {
		return ((const uint8*)imageSizePatterns + sizeof(usbvc_image_size_pattern)
			* numImageSizePatterns + sizeof(uint8));
	} 
} _PACKED;

struct usbvc_color_matching_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	colorPrimaries;
	uint8	transferCharacteristics;
	uint8	matrixCoefficients;
} _PACKED;


struct usbvc_interface_header_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint16	version;
	uint16	totalLength;
	uint32	clockFrequency;
	uint8	numInterfacesNumbers;
	uint8	interfaceNumbers[0];
} _PACKED;

struct usbvc_input_terminal_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	terminalID;
	uint16	terminalType;
	uint8	associatedTerminal;
	uint8	terminal;
} _PACKED;

struct usbvc_output_terminal_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	terminalID;
	uint16	terminalType;
	uint8	associatedTerminal;
	uint8	sourceID;
	uint8	terminal;
} _PACKED;

struct usbvc_camera_terminal_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	terminalID;
	uint16	terminalType;
	uint8	associatedTerminal;
	uint8	terminal;
	uint16	objectiveFocalLengthMin;
	uint16	objectiveFocalLengthMax;
	uint16	ocularFocalLength;
	uint8	controlSize;
	uint8	controls[0];
} _PACKED;

struct usbvc_selector_unit_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	unitID;
	uint8	numInputPins;
	uint8	sourceID[0];
	uint8	Selector() const { return sourceID[numInputPins]; }
} _PACKED;

struct usbvc_processing_unit_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	unitID;
	uint8	sourceID;
	uint16	maxMultiplier;
	uint8	controlSize;
	uint8	controls[0];
	uint8	Processing() const { return controls[controlSize]; }
	uint8	VideoStandards() const { return controls[controlSize+1]; }
} _PACKED;

struct usbvc_extension_unit_descriptor {
	uint8	length;
	uint8	descriptorType;
	uint8	descriptorSubtype;
	uint8	unitID;
	usbvc_guid	guidExtensionCode;
	uint8	numControls;
	uint8	numInputPins;
	uint8	sourceID[0];
	uint8	ControlSize() const { return sourceID[numInputPins]; }
	const uint8*	Controls() const { return &sourceID[numInputPins+1]; }
	uint8	Extension() const
		{	return sourceID[numInputPins + ControlSize() + 1]; }
} _PACKED;

struct usbvc_probecommit {
	uint16	hint;
	uint8	formatIndex;
	uint8	frameIndex;
	uint32	frameInterval;
	uint16	keyFrameRate;
	uint16	pFrameRate;
	uint16	compQuality;
	uint16	compWindowSize;
	uint16	delay;
	uint32	maxVideoFrameSize;
	uint32	maxPayloadTransferSize;
	uint32	clockFrequency;
	uint8	framingInfo;
	uint8	preferredVersion;
	uint8	minVersion;
	uint8	maxVersion;
	void	SetFrameInterval(uint32 interval)
		{ frameInterval = interval; }
} _PACKED;


#endif /* _USB_VIDEO_H */
