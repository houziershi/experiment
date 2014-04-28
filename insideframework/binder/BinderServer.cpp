#define LOG_TAG "BinderServer"

#include <stdlib.h>
#include <fcntl.h>

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include "IBinderService.h"

class BinderService : public BnBinderService
{
public:
	BinderService()
	{
		value = 10;
		LOGE("BinderService create()\n");
	}

	virtual ~BinderService()
	{
		LOGE("BinderService clear()\n");
	}

public:
	static void instantiate()
	{
		defaultServiceManager()->addService(String16(BINDER_SERVICE), new BinderService());
	}

	int32_t getVal()
	{
		return value;
	}

	void setVal(int32_t val)
        {
                value = val;
        }

private:
	int32_t value;
};

int main(int argc, char** argv)
{
	BinderService::instantiate();

	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();

	return 0;
}
