#include <arch/real_time_clock.h>

#include <real_time_clock.h>
#include <real_time_data.h>

#include <Htif.h>


status_t
arch_rtc_init(kernel_args *args, struct real_time_data *data)
{
	bigtime_t systemTime = system_time();
	uint64 rtcTime = HtifCmd(2, 0, 0);
	data->arch_data.system_time_conversion_factor = rtcTime - systemTime;
	return B_OK;
}


uint32
arch_rtc_get_hw_time(void)
{
	return (uint32)(HtifCmd(2, 0, 0) / 1000000);
}


void
arch_rtc_set_hw_time(uint32 seconds)
{
}


void
arch_rtc_set_system_time_offset(struct real_time_data *data, bigtime_t offset)
{
	atomic_set64(&data->arch_data.system_time_offset, offset);
}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data *data)
{
	return atomic_get64(&data->arch_data.system_time_offset);
}
