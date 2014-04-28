#ifndef IBINDERSERVICE_H_
#define IBINDERSERVICE_H_

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#define BINDER_SERVICE "BinderService"

using namespace android;

class IBinderService: public IInterface
{
public:
	DECLARE_META_INTERFACE(BinderService);
	virtual int32_t getVal() = 0;
	virtual void setVal(int32_t val) = 0;
};

class BnBinderService: public BnInterface<IBinderService>
{
public:
	virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
};

#endif
