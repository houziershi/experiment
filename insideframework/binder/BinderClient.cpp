#define LOG_TAG "BinderClient"

#include <utils/Log.h>
#include <binder/IServiceManager.h>

#include "IBinderService.h"

int main()
{
	sp<IBinder> binder = defaultServiceManager()->getService(String16(BINDER_SERVICE));
	if(binder == NULL) {
		LOGE("Failed to get Binder service: %s.\n", BINDER_SERVICE);
		return -1;
	}

	sp<IBinderService> service = IBinderService::asInterface(binder);
	if(service == NULL) {
		LOGE("Failed to get Binder service interface.\n");
		return -2;
	}

	printf("Read original value from BinderService:\n");

	int32_t val = service->getVal();
	printf(" %d.\n", val);

	printf("Add value 1 to BinderService.\n");

	val += 1;
	service->setVal(val);

	printf("Read the value from BinderService again:\n");

	val = service->getVal();
        printf(" %d.\n", val);

	return 0;
}
