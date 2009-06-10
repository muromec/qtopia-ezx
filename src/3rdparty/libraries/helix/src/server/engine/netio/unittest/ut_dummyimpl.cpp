#include "hxcom.h"
#include "hxtypes.h"

//#include "ihxpckts.h"
//#include "hxdtcvt.h"
//#include "dtcvtcon.h"

#include "pcktflowwrap.h"
#include "server_version.h"

HX_RESULT
PacketFlowWrapper::RegisterSource(IHXPSourceControl* pSourceCtrl,
                                  REF(IHXPacketFlowControl*) pPacketFlowControl,
                                  IHXSessionStats* pSessionStats,
                                  UINT16 unStreamCount,
                                  BOOL bUseMDP,
                                  BOOL bIsLive,
                                  BOOL bIsMulticast,
                                  DataConvertShim* pDataConv)
{
    return HXR_NOTIMPL;
}

const char*
ServerVersion::ProductName()
{
        return "unit-testing";
}

// XXX:TDK This is really evil. Pfui!
class DataConvertShim
{
public:
    void Done ();
};

void
DataConvertShim::Done()
{
}

// XXX:TDK This is really evil. Pfui!
class Process;
class Config
{
public:
    const char* GetString(Process*, const char*) const;
};

const char*
Config::GetString(Process*, const char*) const
{
    return "unit-testing";
}

