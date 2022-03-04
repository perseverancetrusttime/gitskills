/** @file ni.c
 *  @brief NextInput driver.
 *
 *  Basic functions user needs to use NextInput AFE
 *
 *  @author Jerry Hung
 *  @bug You tell me
 */



#include "ni_hal.h"
//#include "ni.h"


#ifdef __cplusplus
extern "C" {
#endif

static u16 NI_CAL_CONSTANT[NI_SENSOR_NUM]={0};
static float NI_CAL_FORCE = 2.0;// unit N
//static u8 start_cal_button_flag = 0;		// 当外部设备发送开始校准消息给耳机，把该标志位设置?1
//static u8 load_onoff_flag = 0;			// 外部设备通知耳机负载情况，有负载则该标志位赋值为1，否则赋值为0


u8 ni_init(void)
{
	u8 error = NI_OK;
	u8 s, buffer, i;

	if (sizeof(sensor_list) == 0) {
		return NI_ERROR_NON;
	}

	error = ni_hal_i2c_init();
	if(error != 0)
		return NI_ERROR_COM;
	
	//Test I2C communication with all sensors
	for (s = 0; s < NI_SENSOR_NUM; s++) {
		for ( i = 0; i < MAX_RETRIES; i++) {
			if (ni_hal_i2c_read( sensor_list[s], WAIT_REG, 1, &buffer) == NI_OK) {
				break;
			}

			if (i >= MAX_RETRIES) {
				return NI_ERROR_OVR;
			}
		}
	}

#ifdef REG_INIT

	// Writing the initial values to the clients
	for (i = 0; i < REG_INIT; i++) {
		for (s = 0; s < NI_SENSOR_NUM; s++) {
			error |= ni_hal_i2c_write(sensor_list[s], reg_init[i][0], 1, &(reg_init[i][1]));
		}
	}
	
	if(error != 0)
		return NI_ERROR_COM;
		
#endif

	error = ni_hal_read_calbiraiton_data(NI_SENSOR_NUM, NI_CAL_CONSTANT);
	if(error == 0){
		error = ni_load_calibration_data(NI_CAL_FORCE);
		if(error)
			return NI_ERROR_LOAD_DATA;
	}
	
	return error;
}


u8 ni_read_ADC( u8 sensor_id, s16 *ni_adc )
{
	u8 error = NI_OK;
	u8 buf[2], i;
	s16 adc_data=0;

	if (sensor_id < NI_SENSOR_NUM) { //Read ADC from designed sensor
		error |= ni_hal_i2c_read( sensor_list[sensor_id], ADCOUT_REG, 2, buf);
		adc_data = (s16)((buf[0] << ADCOUT_SHIFT) | (buf[1] >> ADCOUT_SHIFT));

                if(adc_data > 0x800) {
                  adc_data = adc_data-0x1000;
                }

		*ni_adc = adc_data;
	} else { //Read ADC from ALL sensors
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_hal_i2c_read( sensor_list[i], ADCOUT_REG, 2, buf);
			adc_data = (s16)((buf[0] << ADCOUT_SHIFT) | (buf[1] >> ADCOUT_SHIFT));
                        if(adc_data > 0x800) {
                          adc_data = adc_data-0x1000;
                        }
			ni_adc[i] = adc_data;
		}
	}
	return error;
}


u8 ni_i2c_modify_array(struct ni_sensor_t s, u8 reg, u8 data, u8 mask)
{
	u8 error = NI_OK;
	u8 buf;

	error |= ni_hal_i2c_read(s, reg, 1, &buf);

	buf = ( buf & ~mask ) | data;

	error |= ni_hal_i2c_write(s, reg, 1, &buf);

	return error;
}

u8 ni_i2c_read( u8 sensor_id, u8 reg, u8 length, u8 *data ) {
	return ni_hal_i2c_read(sensor_list[sensor_id], reg, length, data);
}

u8 ni_i2c_write( u8 sensor_id, u8 reg, u8 length, u8 *data ) {
	return ni_hal_i2c_write(sensor_list[sensor_id], reg, length, data);
}


u8 ni_force_baseline( u8 sensor_id )
{
	u8 error = NI_OK;

	if (sensor_id < NI_SENSOR_NUM) { //Read ADC from designed sensor
		error |= ni_i2c_modify_array( sensor_list[sensor_id], FORCEBL_REG, 1<<FORCEBL_POS, FORCEBL_MSK );
	} else {
		
		for (u16 i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_i2c_modify_array( sensor_list[i], FORCEBL_REG, 1<<FORCEBL_POS, FORCEBL_MSK );
		}
	}

	return error;
}


u8 ni_read_baseline( u8 sensor_id, s16* ni_baseline )
{
	u8 error = NI_OK;
	u8 buf[2], i;
	s16 adc_data=0;

	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_hal_i2c_read( sensor_list[sensor_id], BASELINE_REG, 2, buf);
		adc_data = (s16)((buf[0] << BASELINE_SHIFT) | (buf[1] >> BASELINE_SHIFT));
		if(adc_data & 0x800) {
			adc_data = -(adc_data & 0x7ff);
		}
		*ni_baseline = adc_data;
	} else {
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_hal_i2c_read( sensor_list[i], BASELINE_REG, 2, buf);
			adc_data = (s16)((buf[0] << BASELINE_SHIFT) | (buf[1] >> BASELINE_SHIFT));
			if(adc_data & 0x800) {
				adc_data = -(adc_data & 0x7ff);
			}			
			ni_baseline[i]= adc_data;
		}
	}

	return error;
}

u8 ni_read_interrupt_threshold( u8 sensor_id, u16* ni_interrupt_threshold )
{
	u8 error = NI_OK;
	u8 buf[2], i;

	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_hal_i2c_read( sensor_list[sensor_id], INTRTHRSLD_REG, 2, buf);
		*ni_interrupt_threshold = (u16)((buf[0] << INTRTHRSLD_SHIFT) | (buf[1] >> INTRTHRSLD_SHIFT));
	} else {
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_hal_i2c_read( sensor_list[i], INTRTHRSLD_REG, 2, buf);
			ni_interrupt_threshold[i] = (u16)((buf[0] << INTRTHRSLD_SHIFT) | (buf[1] >> INTRTHRSLD_SHIFT));
		}
	}

	return error;
}


u8 ni_read_autocal_thr( u8 sensor_id, u16* ni_autocal_thr )
{
	u8 error = NI_OK;
	u8 buf[2], i;

	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_hal_i2c_read( sensor_list[sensor_id], AUTOCAL_THR_REG, 2, buf);
		*ni_autocal_thr = (u16)((buf[0] << AUTOCAL_THR_SHIFT) | (buf[1] >> AUTOCAL_THR_SHIFT));
	} else {
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_hal_i2c_read( sensor_list[i], AUTOCAL_THR_REG, 2, buf);
			ni_autocal_thr[i] = (u16)((buf[0] << AUTOCAL_THR_SHIFT) | (buf[1] >> AUTOCAL_THR_SHIFT));
		}
	}

	return error;
}


u8 ni_set_interrupt_threshold( u8 sensor_id, u16* ni_interrupt_threshold )
{
	u8 error = NI_OK;
	u8 buf[2] = {0};

	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_hal_i2c_read( sensor_list[sensor_id], INTRTHRSLD_REG+1, 1, buf+1);

		if (error != NI_OK) {
			return error;
		}

		buf[0] = (u8)(*ni_interrupt_threshold >> INTRTHRSLD_SHIFT);
		buf[1] &= 0x0F;
		buf[1] |= (u8)(*ni_interrupt_threshold << INTRTHRSLD_SHIFT);
		error |= ni_hal_i2c_write(sensor_list[sensor_id], INTRTHRSLD_REG, 2, buf);
	} else {
		error = NI_ERROR_NON;
	}

	return error;
}


u8 ni_set_fallthr( u8 sensor_id, u16* fallthr )
{
	u8 error = NI_OK;
	u8 buf[2] = {0};

	if (sensor_id < NI_SENSOR_NUM) {
		buf[0] = (u8)(*fallthr >> FALLTHR_SHIFT);
		buf[1] |= (u8)(*fallthr << FALLTHR_SHIFT);
		error |= ni_hal_i2c_write(sensor_list[sensor_id], FALLTHR_REG, 2, buf);
	} else {
		error = NI_ERROR_NON;
	}

	return error;
}


u8 ni_set_autocal_thr( u8 sensor_id, u16* ni_autocal_thr )
{
	u8 error = NI_OK;
	u8 buf[2]={0};

	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_hal_i2c_read( sensor_list[sensor_id], AUTOCAL_THR_REG+1, 1, buf+1);

		if (error != NI_OK) {
			return error;
		}

		buf[0] = (u8)(*ni_autocal_thr >> AUTOCAL_THR_SHIFT);
		buf[1] &= 0x0F;
		buf[1] |= (u8)(*ni_autocal_thr << AUTOCAL_THR_SHIFT);
		error |= ni_hal_i2c_write(sensor_list[sensor_id], AUTOCAL_THR_REG, 2, buf);
	} else {
		error = NI_ERROR_NON;
	}

	return error;
}

u8 ni_write_reg(u8 sensor_id, u8 data, u8 reg, u8 shift, u8 mask ) {
	u8 error = NI_OK;
	u8 buf;

	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_hal_i2c_read( sensor_list[sensor_id], reg, 1, &buf);

		if (error != NI_OK) {
			return error;
		}

		buf &= ~mask;
		buf |= data << shift;
		error |= ni_hal_i2c_write(sensor_list[sensor_id], reg, 1, &buf);
	} else {
		error = NI_ERROR_NON;
	}

	return error;
}

u8 ni_set_interrupt_enable(u8 sensor_id) {
	u8 error = NI_OK;
	u8 i = 0;
	
	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_i2c_modify_array( sensor_list[sensor_id], INTREN_REG, 1<<INTREN_POS, BTNMODE_MSK );
	} else {
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_i2c_modify_array( sensor_list[i], INTREN_REG, 1<<INTREN_POS, BTNMODE_MSK );
		}
	}

	return error;
}

u8 ni_set_interrupt_disable(u8 sensor_id) {
	u8 error = NI_OK;
	u8 i = 0;
	
	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_i2c_modify_array( sensor_list[sensor_id], INTREN_REG, 0<<INTREN_POS, BTNMODE_MSK );
	} else {
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_i2c_modify_array( sensor_list[i], INTREN_REG, 0<<INTREN_POS, BTNMODE_MSK );
		}
	}

	return error;
}

u8 ni_set_adcraw_bl(u8 sensor_id) {
	u8 error = NI_OK;
	u8 i = 0;
	
	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_i2c_modify_array( sensor_list[sensor_id], ADCRAW_REG, ADCRAW_BL, ADCRAW_MSK );
	} else {
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_i2c_modify_array( sensor_list[i], ADCRAW_REG, ADCRAW_BL, ADCRAW_MSK );
		}
	}

	return error;
}

u8 ni_set_adcraw_raw(u8 sensor_id) {
	u8 error = NI_OK;
	u8 i = 0;
	
	if (sensor_id < NI_SENSOR_NUM) {
		error |= ni_i2c_modify_array( sensor_list[sensor_id], ADCRAW_REG, ADCRAW_RAW<<ADCRAW_POS, ADCRAW_MSK );
	} else {
		for (i = 0; i < NI_SENSOR_NUM; i++) {
			error |= ni_i2c_modify_array( sensor_list[i], ADCRAW_REG, ADCRAW_RAW<<ADCRAW_POS, ADCRAW_MSK );
		}
	}

	return error;
}





u8 ni_calibration_init(s16 *sensor_init_offset) 
{
	u8 error = NI_OK;
	u16 i=0,j=0;
	s16 sensor_data[NI_SENSOR_NUM]={0};
	s16 adc_data[NI_SENSOR_NUM];
	u8 sensorID=0;
	
	if(NI_SENSOR_NUM==1)
		sensorID=0;
	else sensorID=NI_SENSOR_NUM;
	
	error |= ni_set_interrupt_disable(sensorID);//disable BTN mode
	error |= ni_set_adcraw_raw(sensorID);//disable baseline algo

	i=0;
	do{
		ni_hal_delay(10);//10ms
		ni_read_ADC(sensorID, adc_data);
		++i;
	}while((adc_data[0]==0)&&(i<100));

	/*Get sensor initial raw data*/
	for(i=0; i<average_sample_num; ++i){
		error |= ni_read_ADC(sensorID,adc_data);
		for(j=0;j<NI_SENSOR_NUM;++j)
			sensor_data[j]+=adc_data[j];
	}
	for(j=0;j<NI_SENSOR_NUM;++j)
		sensor_init_offset[j]=sensor_data[j]/average_sample_num;

		
	return error;
}

u8 ni_calibration_start(s16 *sensor_load_adc) 
{
	u8 error = NI_OK;
	u16 i=0,j=0;
	s16 sensor_data[NI_SENSOR_NUM]={0};
	s16 adc_data[NI_SENSOR_NUM];
	u8 sensorID=0;
	
	if(NI_SENSOR_NUM==1)
		sensorID=0;
	else sensorID=NI_SENSOR_NUM;

	/* start to get sensor data with weight on the kit*/
	for(i=0;i<average_sample_num;++i){
		error |= ni_read_ADC(sensorID,adc_data);
		for(j=0;j<NI_SENSOR_NUM;++j)
			sensor_data[j]+=adc_data[j];
	}
	
	for(j=0;j<NI_SENSOR_NUM;++j)
		sensor_load_adc[j]=sensor_data[j]/average_sample_num;
		
	
	return error;
}

u8 ni_load_calibration_data(float desired_force)
{
	u8 error = NI_OK;
	u16 ni_interrupt_threshold[NI_SENSOR_NUM] = {0};
	u16 ni_autocal_thr[NI_SENSOR_NUM] = {0};
	u8 i = 0;

	for (i = 0; i < NI_SENSOR_NUM; i++) {
		ni_interrupt_threshold[i] = (u16)(desired_force * NI_CAL_CONSTANT[i]);
		ni_autocal_thr[i] = (u16)(0.8 * ni_interrupt_threshold[i]);
		
		error |= ni_set_interrupt_threshold( i, &ni_interrupt_threshold[i] );
		error |= ni_set_autocal_thr(i, &ni_autocal_thr[i] );
	}
	 
	return error;
}

u8 ni_calc_sensor_cal_data(u8 sensor_num, u16 *sensor_cal_data, s16 * sensor_load_adc, s16 * sensor_initial_adc) 
{
	u8 error = NI_OK;
	s16 delta = 0;
	u8 i = 0;
	//Calculate and save calibration Constant c(in counts/N)  c = (aL-a0) / LC
	for (i=0; i<sensor_num; ++i){
		delta = sensor_load_adc[i] - sensor_initial_adc[i];
		
		if(delta < NI_INTR_MIN_THRESHOLD){
			return NI_ERROR_CAL_DATA_MIN;
		}
		
		sensor_cal_data[i] = (u16)(delta / NI_CAL_FORCE);
	}
	
	return error;
}

u8 ni_calibration_done(s16 * sensor_load_adc, s16 * sensor_initial_adc)
{
	u8 error = NI_OK;
	//u8 sensor_id=0;
	u16 i = 0;
	u16 sensor_cal_data[NI_SENSOR_NUM]={0};

	//Calculate and save calibration Constant c(in counts/N)  c = (aL-a0) / LC
	error = ni_calc_sensor_cal_data(NI_SENSOR_NUM, sensor_cal_data, sensor_load_adc, sensor_initial_adc);
	if(error)
		return NI_ERROR_CAL_DATA_MIN;
		
	error = ni_hal_write_calbiraiton_data(NI_SENSOR_NUM, sensor_cal_data);
	if(error)
		return NI_ERROR_CAL_DATA_W;
	
	
	error = ni_hal_read_calbiraiton_data(NI_SENSOR_NUM, NI_CAL_CONSTANT);
	if(error)
		return NI_ERROR_CAL_DATA_R;
	
	for (i=0; i < NI_SENSOR_NUM; ++i) {
		if( NI_CAL_CONSTANT[i] != sensor_cal_data[i])
			return NI_ERROR_STORER;
	}
	
	error = ni_load_calibration_data(NI_CAL_FORCE);
	if(error)
		return NI_ERROR_LOAD_DATA;
	
	error |= ni_set_interrupt_enable(NI_SENSOR_NUM);
	error |= ni_force_baseline(NI_SENSOR_NUM);
	
	return error;
}

u8 ni_calibration_process(void)
{
	u8 error = NI_OK;
/*	
	static u8 step = 0;
	static s16 sensor_initial_adc[NI_SENSOR_NUM]={0};
	static s16 sensor_load_adc[NI_SENSOR_NUM]={0};
	
	switch (step)
	{
		case 0:
			if (start_cal_button_flag) {
				step++;
				start_cal_button_flag = 0;
			}
			break;
		
		case 1:
			// read load_onoff_flag
			if (load_onoff_flag == 0) {
				error |= ni_calibration_init(sensor_initial_adc);
				if (error)
					error = NI_ERROR_INIT_ADC;
				step++;
			}
			else 
				error = NI_ERROR_INIT_LOAD;
			break;	
		
	    case 2:
			// read load_onoff_flag
			if (load_onoff_flag == 1) {
				step++;
			}
			break;
			
	    case 3:
			ni_hal_delay(500);
			step++;
			break;	
			
		case 4:
			error |= ni_calibration_start(sensor_load_adc);
			step++;
			break;	
		
	    case 5:
			error |= ni_calibration_done(sensor_load_adc, sensor_initial_adc);
			
			// send message to reset load
			
			if(error == 0){
				//send message calibration succeed
			}
			step = 0;
			break;
		
		
	    default:
		
			// send message to reset load
			
			step = 0;
			break;
	}

	if(error){
		step = 0;
		// send message to reset load
		// send message report calibrion failed.
	}
*/
	return error;
}


#ifdef __cplusplus
}
#endif
