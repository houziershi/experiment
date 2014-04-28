#define LOG_TAG "IBinderService"

#include <utils/Log.h>

#include "IBinderService.h"

using namespace android;

enum
{
	GET_VAL = IBinder::FIRST_CALL_TRANSACTION,
	SET_VAL
};

class BpBinderService: public BpInterface<IBinderService>
{
public:
	BpBinderService(const sp<IBinder>& impl)
		: BpInterface<IBinderService>(impl)
	{

	}

public:
	int32_t getVal()
	{
		Parcel data;
		data.writeInterfaceToken(IBinderService::getInterfaceDescriptor());

		Parcel reply;
		remote()->transact(GET_VAL, data, &reply);

		int32_t val = reply.readInt32();

		return val;
	}

	void setVal(int32_t val)
        {
                Parcel data;
                data.writeInterfaceToken(IBinderService::getInterfaceDescriptor());
		data.writeInt32(val);

                Parcel reply;
                remote()->transact(SET_VAL, data, &reply);
        }

};

IMPLEMENT_META_INTERFACE(BinderService, "BinderService");

status_t BnBinderService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
	switch(code)
	{
		case GET_VAL:
		{
			CHECK_INTERFACE(IBinderService, data, reply);

			int32_t val = getVal();
			reply->writeInt32(val);

			return NO_ERROR;
		}
		case SET_VAL:
                {
                        CHECK_INTERFACE(IBinderService, data, reply);

			int32_t val = data.readInt32();
			setVal(val);

                        return NO_ERROR;
                }
		default:
		{
			return BBinder::onTransact(code, data, reply, flags);
		}
	}
}
