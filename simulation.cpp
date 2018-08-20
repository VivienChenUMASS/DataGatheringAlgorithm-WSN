//
//  simulation.cpp
//
//  Created by Vivien Chen.
//  Copyright (c) Vivien Chen. All rights reserved.
//

#include "simulation.hpp"
#include "SPA.hpp"

float Request_Model::generate_PossionInterval(){
    
    float interval;
    interval = -logf(genrand_real2())/lambda*Operation_Period / 60; //here, should be minutes
    assert(interval >= 0);
    
    return interval;
}

float Energy_Harvest_Model::get_EnergyHarvestRate(TIME harvestTime){
    
    float coefficient = 0.01;
    float HarvestRate = -1000;
    //double incidence;
    double cos_i;
    
    spa_data spa;  //declare the SPA structure
    int result;
    //float min, sec;
    
    //enter required input values into SPA structure
    
    spa.year          = 2003;
    spa.month         = 10;
    spa.day           = harvestTime.day;
    spa.hour          = harvestTime.hour;
    spa.minute        = harvestTime.minute;
    spa.second        = 30;
    spa.timezone      = -7.0;
    spa.delta_ut1     = 0;
    spa.delta_t       = 67;
    spa.longitude     = -105.1786;
    spa.latitude      = 39.742476;
    spa.elevation     = 1830.14;
    spa.pressure      = 820;
    spa.temperature   = 11;
    spa.slope         = 30;
    spa.azm_rotation  = -10;
    spa.atmos_refract = 0.5667;
    spa.function      = SPA_ZA_INC;
    
    //call the SPA calculate function and pass the SPA structure
    
    result = spa_calculate(&spa);
    
    if (result == 0)  //check for SPA errors
    {
        
        cos_i = cos(deg2rad(spa.incidence));
        //std::cout << "cos_i is: " << cos_i << std::endl;
        
        if (cos_i <= 0) {
            
            HarvestRate = 0;
        }
        else{
            
            HarvestRate = coefficient * cos_i;
        }
        
        
    } else{
        
        printf("SPA Error Code: %d\n", result);
    }
    
    return HarvestRate;  //J/s
    
}

float Energy_Harvest_Model::get_Harvest_Energy(TIME lastUpdateT, TIME currentT){
    
    float Harvest_Energy = 0.0;
    
    //since every minute energy harvest rate is changing, we need to do a sum of every different minute
    TIME t = lastUpdateT;
    
    while (t != currentT){
        
        Harvest_Energy = Harvest_Energy + get_EnergyHarvestRate(t) * 1;
        if (t.minute + 1 == 60) {
            
            t.minute = 0;
            if (t.hour + 1 == 24) {
                
                t.hour = 0;
                t.day = t.day + 1; //here, we assume the day will be still in the same month!!!!!!
            }
            else{
                
                t.hour = t.hour + 1;
            }
        }
        else{
            
            t.minute = t.minute + 1;
        }
    }
    
    return Harvest_Energy;
}


float Temperature_Model::generate_temperature(){
    float temperature;
    
    temperature = 60 * genrand_real1() - 30; //-30 ~ 30
    
    return temperature;
}


float User::Value_of_Data(float absolute_error, float actual_dvalue){
    
    float value_of_data;
    
    float relative_error = absolute_error / (fabs(actual_dvalue) + 1);
    
    //cout << "relative error is : " << relative_error << endl;
    value_of_data = -100 * pow(relative_error, 2);
    return value_of_data;
    
}

REQUEST* User::generate_nextRequest(){
    
    float interval;
    TIME nextReqTime;
    REQUEST *nextReq;
    
    interval = User_Request_Model.generate_PossionInterval();
    
    nextReqTime = Simulation_Current_Time;
    int nextReqTime_Minute = nextReqTime.day * 24 * 60 + nextReqTime.hour * 60 + nextReqTime.minute;
    nextReqTime_Minute = nextReqTime_Minute + interval; //minutes
    
    nextReqTime.minute = nextReqTime_Minute % 60;
    nextReqTime.hour = (nextReqTime_Minute / 60) % 24;
    nextReqTime.day = (nextReqTime_Minute / 60) / 24;
    
    nextReq = (REQUEST *)malloc(sizeof(REQUEST));
    (*nextReq).time = nextReqTime;
    //generate a position in the Area
    (*nextReq).request_position.x = (Area_End.x - Area_Start.x) * genrand_real2();
    (*nextReq).request_position.y = (Area_End.y - Area_Start.y) * genrand_real2();
    
    //cout << "next Request x is " << nextReq->request_position.x << " y is " << nextReq->request_position.y << endl;
    
    return nextReq;
}

void BaseStation::find_firstRequest(){
    
    for (map<int, REQUEST*>::iterator it = UserID_RequestHead.begin(); it != UserID_RequestHead.end(); ++it) {
        
        if (FirstReq == NULL) {
            FirstReq = it->second;
            FirstReq_UserID = it->first;
        }
        else{
            if ((FirstReq->time.day*24*60+FirstReq->time.hour*60+FirstReq->time.minute) > (it->second->time.day*24*60+it->second->time.hour*60+it->second->time.minute)) {
                
                FirstReq = it->second;
                FirstReq_UserID = it->first;
            }
        }
    }
    //when find the next first request to be served, the SensorID_Available will be reset!!!
    SensorID_Available_Algorithm1.clear();
    SensorID_Available_Algorithm2.clear();
    SensorID_Available_Algorithm3.clear();
    
    for (int i = 1; i <= SensorNumber; i++) {
        //here, we will adjust the available sensors according to the sense range and the request location!!!!!!
        float distance_pow;
        distance_pow = pow((SensorID_SensorP[i]->Sensor_Position.x - FirstReq->request_position.x),2) + pow((SensorID_SensorP[i]->Sensor_Position.y - FirstReq->request_position.y), 2);
        if (distance_pow < pow(SensorID_SensorP[i]->Sensing_Range, 2)) {
            
            SensorID_Available_Algorithm1.push_back(i);
            SensorID_Available_Algorithm2.push_back(i);
            SensorID_Available_Algorithm3.push_back(i);
            
        }
    }
}

void BaseStation::update_LastRepData_Algorithm(int Algorithm_NO, float RepData, int sensor_id){
    
    switch (Algorithm_NO) {
        case 1:
            //cout << "Algorithm1: Last Report Data is: " << LastRepData_Algorithm1[sensor_id - 1] << endl;
            LastRepData_Algorithm1[sensor_id - 1] = RepData;
            //cout << "Algorithm1: Update Last Report Data to be: " << LastRepData_Algorithm1[sensor_id - 1] << endl;
            break;
        case 2:
            //cout << "Algorithm2: Last Report Data is: " << LastRepData_Algorithm2[sensor_id - 1] << endl;
            LastRepData_Algorithm2[sensor_id - 1] = RepData;
            //cout << "Algorithm2: Update Last Report Data to be: " << LastRepData_Algorithm2[sensor_id - 1] << endl;
            break;
        case 3:
            //cout << "Algorithm3: Last Report Data is: " << LastRepData_Algorithm3[sensor_id - 1] << endl;
            LastRepData_Algorithm3[sensor_id - 1] = RepData;
            //cout << "Algorithm3: Update Last Report Data to be: " << LastRepData_Algorithm3[sensor_id - 1] << endl;
            break;
        default:
            cout << "No Algorithm Case Match!" << endl;
            exit(1);
    }
}

void BaseStation::update_LastRepDataTime_Algorithm(int Algorithm_NO, TIME LastRepDataTime, int sensor_id){
    
    switch (Algorithm_NO) {
        case 1:
            LastRepDataTime_Algorithm1[sensor_id - 1] = LastRepDataTime;
            break;
        case 2:
            LastRepDataTime_Algorithm2[sensor_id - 1] = LastRepDataTime;
            break;
        case 3:
            LastRepDataTime_Algorithm3[sensor_id - 1] = LastRepDataTime;
            break;
        default:
            cout << "No Algorithm Case Match!" << endl;
            exit(1);
    }
    
}

void BaseStation::send_Request_Algorithm(int Algorithm_NO){
    //handle the first request in the base station
    POSITION Req_Position = FirstReq->request_position;
    float Min_Distance = 1000000000; //make the initial distance very big!!!!!
    //get the location needed from the request and send the request to the nearest sensor
    if (Algorithm_NO == 1) {
        for (map<int, Sensor*>::iterator it = SensorID_SensorP.begin(); it != SensorID_SensorP.end(); ++it){
            //check if the sensor has been requested!!!
            if (find(SensorID_Available_Algorithm1.begin(), SensorID_Available_Algorithm1.end(), it->first) !=  SensorID_Available_Algorithm1.end()) {
                float temp_distance = pow((it->second->Sensor_Position.x - Req_Position.x), 2) + pow((it->second->Sensor_Position.y - Req_Position.y), 2);
                if (temp_distance < Min_Distance) {
                    Min_Distance = temp_distance;
                    SensorID_Requested = it->first;
                }
            }
        }
    }
    if (Algorithm_NO == 2) {
        for (map<int, Sensor*>::iterator it = SensorID_SensorP.begin(); it != SensorID_SensorP.end(); ++it) {
            //check if the sensor has been requested!!!
            if (find(SensorID_Available_Algorithm2.begin(), SensorID_Available_Algorithm2.end(), it->first) != SensorID_Available_Algorithm2.end()) {
                float temp_distance = pow((it->second->Sensor_Position.x - Req_Position.x), 2) + pow((it->second->Sensor_Position.y - Req_Position.y), 2);
                if (temp_distance < Min_Distance) {
                    Min_Distance = temp_distance;
                    SensorID_Requested = it->first;
                }
            }
        }
    }
    if (Algorithm_NO == 3) {
        for (map<int, Sensor*>::iterator it = SensorID_SensorP.begin(); it != SensorID_SensorP.end(); ++it) {
            //check if the sensor has been requested!!!
            if (find(SensorID_Available_Algorithm3.begin(), SensorID_Available_Algorithm3.end(), it->first) != SensorID_Available_Algorithm3.end()) {
                float temp_distance = pow((it->second->Sensor_Position.x - Req_Position.x), 2) + pow((it->second->Sensor_Position.y - Req_Position.y), 2);
                if (temp_distance < Min_Distance) {
                    Min_Distance = temp_distance;
                    SensorID_Requested = it->first;
                }
            }
        }
    }
    
}

void BaseStation::delete_Req(){
    
    free(FirstReq);
    FirstReq = NULL;
}

void Sensor::process_request_Algorithm(int Algorithm_NO, REQUEST request){
    
    switch (Algorithm_NO) {
        case 1: //Algorithm1: Process request reffering to a Decision Table
        {
            //here will according to the request, then to decide whether to transmit or not
            //after decision, do not forget to change the related parameters of the sensor
            
            //Total_Request_NO = Total_Request_NO + 1;
            
            //moving the clock to the request handling time
            Simulation_Current_Time = request.time;
            //recaculating the energy remaining on the sensor, since there is a factor of energy harvesting, but the total energy remained on the sensor could not be more than the capacity
            float harvest_energy = EHarvestM_p->get_Harvest_Energy(Last_EHarvest_UpdateT_Algorithm1, Simulation_Current_Time);
            Last_EHarvest_UpdateT_Algorithm1 = Simulation_Current_Time; //update it
            
            float temp_energy = Energy_Remain_Algorithm1 + harvest_energy;
            if (temp_energy > Capacity) {
                Energy_Remain_Algorithm1 = Capacity;
            }
            else{
                Energy_Remain_Algorithm1 = temp_energy;
            }
            //here, we assume the sensing action spends no energy and do not need any energy, later we may change this
            Current_Sensed_Data = Sense_Current_Data(request.request_position);
            //Round_Sensed_Flag = 1; //set the flag to show that the sensor has sensed the data this round
            
            //if the energy remained is less than the enery need for transmiting action, there is no need to refer to the table, the decision must be not transmit, only when it is more than the trans action energy, there is a need to refer to the table
            
            if (Energy_Remain_Algorithm1 >= Transmit_Action_Energy) {
                
                if (Last_Report_Data_Algorithm1 == -10000000000) {  //which means this request is the first request of the whole system
                    Trans_Decision = 1;
                }
                else{
                    float Time_Level_Interval = Delta_T / 60; //minute
                    
                    int timeFromStart = Simulation_Current_Time.day * 24 * 60 + Simulation_Current_Time.hour * 60 +     Simulation_Current_Time.minute - (Simulation_Start_Time.day * 24 * 60 + Simulation_Start_Time.hour * 60 + Simulation_Start_Time.minute);  //minutes
                    
                    int time_level = timeFromStart / Time_Level_Interval;
                    int energy_level = Energy_Remain_Algorithm1 / Energy_Level_Interval;
                    int lrdata_level = Last_Report_Data_Algorithm1 / Temperature_Difference_Min + 30 / 2;
                    int cdata_level = Current_Sensed_Data / Temperature_Difference_Min + 30 / 2;
                    
                    Trans_Decision = decision_Table_Algorithm1[time_level][energy_level][lrdata_level][cdata_level]; //according to the current time, the energy remaining, last reported data and the current sensed fresh data
                }
            }
            else{
                
                Trans_Decision = 0; // Not Transmit
            }
            
            if (Trans_Decision == 1) { //TRANSMIT
                
                Energy_Remain_Algorithm1 = Energy_Remain_Algorithm1 - Transmit_Action_Energy;
                
            }
            /*else{ //NOT TRANSMIT
             
             //Total_NotTrans_NO = Total_NotTrans_NO + 1;
             }*/
            break;
        }
        case 2: //Algorithm2: Process requests in a greedy way, whenever the sensor has the enough energy to transmit back the fresh data, it will transmit back
            
        {   Simulation_Current_Time = request.time;
            //recaculating the energy remaining on the sensor, since there is a factor of energy harvesting, but the total energy remained on the sensor could not be more than the capacity
            float harvest_energy = EHarvestM_p->get_Harvest_Energy(Last_EHarvest_UpdateT_Algorithm2, Simulation_Current_Time);
            Last_EHarvest_UpdateT_Algorithm2 = Simulation_Current_Time; //update it
            
            float temp_energy = Energy_Remain_Algorithm2 + harvest_energy;
            if (temp_energy > Capacity) {
                Energy_Remain_Algorithm2 = Capacity;
            }
            else{
                Energy_Remain_Algorithm2 = temp_energy;
            }
            //here, we assume the sensing action spends no energy and do not need any energy, later we may change this
            Current_Sensed_Data = Sense_Current_Data(request.request_position);
            //Round_Sensed_Flag = 1; //set the flag to show that the sensor has sensed the data this round
            
            //if the energy remained is less than the enery need for transmiting action, there is no need to refer to the table, the decision must be not transmit, only when it is more than the trans action energy, there is a need to refer to the table
            
            if (Energy_Remain_Algorithm2 >= Transmit_Action_Energy) {
                
                Trans_Decision = 1; // Transmit back
            }
            else{
                
                Trans_Decision = 0; // Not Transmit
            }
            
            if (Trans_Decision == 1) { //TRANSMIT
                
                Energy_Remain_Algorithm2 = Energy_Remain_Algorithm2 - Transmit_Action_Energy;
                
            }
            
            break;
        }
        case 3: //Algorithm3: Process requests using a static predefined threshold value
        {
            Simulation_Current_Time = request.time;
            //recaculating the energy remaining on the sensor, since there is a factor of energy harvesting, but the total energy remained on the sensor could not be more than the capacity
            float harvest_energy = EHarvestM_p->get_Harvest_Energy(Last_EHarvest_UpdateT_Algorithm3, Simulation_Current_Time);
            Last_EHarvest_UpdateT_Algorithm3 = Simulation_Current_Time; //update it
            
            float temp_energy = Energy_Remain_Algorithm3 + harvest_energy;
            if (temp_energy > Capacity) {
                Energy_Remain_Algorithm3 = Capacity;
            }
            else{
                Energy_Remain_Algorithm3 = temp_energy;
            }
            //here, we assume the sensing action spends no energy and do not need any energy, later we may change this
            Current_Sensed_Data = Sense_Current_Data(request.request_position);
            //Round_Sensed_Flag = 1; //set the flag to show that the sensor has sensed the data this round
            
            if (Energy_Remain_Algorithm3 >= Transmit_Action_Energy) {
                
                float relative_error;
                /*if (Current_Sensed_Data == 0) { // mathematical corner case
                 
                 relative_error = (Last_Report_Data_Algorithm3 - Current_Sensed_Data) / (Current_Sensed_Data + 0.0001);
                 }
                 else{
                 
                 relative_error = (Last_Report_Data_Algorithm3 - Current_Sensed_Data) / Current_Sensed_Data;
                 }*/
                relative_error = fabs(Last_Report_Data_Algorithm3 - Current_Sensed_Data) / (fabs(Current_Sensed_Data) + 1);
                
                if (relative_error > 10) { //if the relative error is bigger than the threshold value, the decision will be transmitting back the fresh data
                    Trans_Decision = 1; // Transmit
                }
                else{
                    
                    Trans_Decision = 0; // Not Transmit
                }
            }
            else{ //when the remained energy is less than the energy needed for transmitting action, the decision must be 0
                
                Trans_Decision = 0; // Not Transmit
            }
            
            if (Trans_Decision == 1) { //TRANSMIT
                
                Energy_Remain_Algorithm3 = Energy_Remain_Algorithm3 - Transmit_Action_Energy;
                
            }
            
            break;
        }
        default:
            cout << "NO ALGORITHM CASE MATCH!" << endl;
            exit(1);
    }
}


float Sensor::Sense_Current_Data(POSITION request_position){
    
    float Sensed_Data;
    //Sensed_Data = TM_p->generate_temperature(request_position.x, request_position.y, Simulation_Current_Time, TM_p->get_Current_Cloudiness());
    if ((TM_p->get_Current_Temperature_Flag()) == 0) {
        Sensed_Data = TM_p->generate_temperature();
        TM_p->set_Current_Temperature(Sensed_Data);
        TM_p->set_Current_Temperature_Flag(1);
        //cout << "This round, temperature generated is : " << Sensed_Data << endl;
    }
    else{
        Sensed_Data = TM_p->get_Current_Temperature();
        //cout << "This round, the second sensor gets the current temperature is : " << Sensed_Data << endl;
    }
    //Round_Sensed_Flag = 1;
    return Sensed_Data;
}

void Sensor::update_Last_Report_Data_Algorithm(int Algorithm_NO, float RepData){
    
    switch (Algorithm_NO) {
        case 1:
            Last_Report_Data_Algorithm1 = RepData;
            break;
        case 2:
            Last_Report_Data_Algorithm2 = RepData;
            break;
        case 3:
            Last_Report_Data_Algorithm3 = RepData;
            break;
        default:
            cout << "No Algorithm Case Match!" << endl;
            exit(1);
    }
}

void SensorNet_System::Initialize_User(User* userToIniP, float operation_period, float lambda){
    
    (*userToIniP).set_Period(operation_period);
    (*userToIniP).set_User_Request_Model(lambda);
    (*userToIniP).set_User_Area(Area_Start, Area_End);
}

void SensorNet_System::Initialize_Sensor(Sensor* sensorToIniP, int position_x, int position_y, float sensing_range, float capacity, float trans_energy, Energy_Harvest_Model* EHp, Temperature_Model* TMp){
    
    (*sensorToIniP).set_Sensor_Position(position_x, position_y); //for this version, the single sensor's position does not matter
    (*sensorToIniP).set_Sensing_Range(sensing_range);
    (*sensorToIniP).set_Capacity(capacity);
    (*sensorToIniP).set_Transmit_Action_Energy(trans_energy);
    (*sensorToIniP).set_Energy_Remain_Algorithm1(capacity);
    (*sensorToIniP).set_Energy_Remain_Algorithm2(capacity);
    (*sensorToIniP).set_Energy_Remain_Algorithm3(capacity);
    
    (*sensorToIniP).set_EHarvestM_p(EHp);
    (*sensorToIniP).set_TM_p(TMp);
    (*sensorToIniP).set_Last_EHarvest_UpdateT_Algorithm1(Simulation_Start_Time);
    (*sensorToIniP).set_Last_EHarvest_UpdateT_Algorithm2(Simulation_Start_Time);
    (*sensorToIniP).set_Last_EHarvest_UpdateT_Algorithm3(Simulation_Start_Time);
    
    (*sensorToIniP).set_Current_Sensed_Data(-1000000); //Have to check later!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    (*sensorToIniP).set_Total_Req_NO_Algorithm(1, 0);
    (*sensorToIniP).set_Total_Req_NO_Algorithm(2, 0);
    (*sensorToIniP).set_Total_Req_NO_Algorithm(3, 0);
    
    (*sensorToIniP).set_Total_NotTrans_NO_Algorithm(1, 0);
    (*sensorToIniP).set_Total_NotTrans_NO_Algorithm(2, 0);
    (*sensorToIniP).set_Total_NotTrans_NO_Algorithm(3, 0);
    
    (*sensorToIniP).Last_Report_Data_Algorithm1 = -10000000000;
    (*sensorToIniP).Last_Report_Data_Algorithm2 = -10000000000;
    (*sensorToIniP).Last_Report_Data_Algorithm3 = -10000000000;
    
    //initialize the decision_Table
    for (int i = 0; i < NO_TimeLevels; i++) {
        vector<vector<vector<int>>> temp;
        for (int j = 0; j < NO_EnergyLevels; j++) {
            vector<vector<int>> temptemp;
            for (int k = 0; k <NO_DataLevels; k++) {
                vector<int> temptemptemp;
                for (int h = 0; h < NO_DataLevels; h++) {
                    temptemptemp.push_back(0);
                }
                temptemp.push_back(temptemptemp);
            }
            temp.push_back(temptemp);
        }
        (*((*sensorToIniP).access_Decision_Table())).push_back(temp);
    }
    
}

void SensorNet_System::Initialize_BaseStation(){
    
    myBaseStation.set_AreaStart(Area_Start.x, Area_Start.y);
    myBaseStation.set_AreaEnd(Area_End.x, Area_End.y);
    myBaseStation.set_subArea(Subarea.width, Subarea.length);
    myBaseStation.set_subAreaNo(Subarea_NO.rowNO, Subarea_NO.columnNO);
    myBaseStation.set_UserNumber(UserNumber);
    myBaseStation.set_SensorNumber(SensorNumber);
    for (int i = 0; i <= 3; i++) {
        myBaseStation.Reward_Gained_Algorithm.push_back(0.0);
    }
    
    //initialize the last report data vector in the base station
    for (int i = 0; i < SensorNumber; i++) {
        myBaseStation.LastRepData_Algorithm1.push_back(-10000000000);
        myBaseStation.LastRepData_Algorithm2.push_back(-10000000000);
        myBaseStation.LastRepData_Algorithm3.push_back(-10000000000);
    }
    for (int i = 0; i < SensorNumber; i++) {
        myBaseStation.LastRepDataTime_Algorithm1.push_back(Simulation_Start_Time);
        myBaseStation.LastRepDataTime_Algorithm2.push_back(Simulation_Start_Time);
        myBaseStation.LastRepDataTime_Algorithm3.push_back(Simulation_Start_Time);
    }
    
    map<int, Sensor*>* SensorID_SensorPP = myBaseStation.access_SensorID_SensorP();
    map<int, REQUEST*>* UserID_RequestHeadP = myBaseStation.access_UserID_RequestHead();
    
    //initialize the last report data vector in the base station
    /*for (int i = 0; i < Subarea_NO.rowNO; i++) {
     vector<float> temp;
     for (int j = 0; j < Subarea_NO.columnNO; j++) {
     
     temp.push_back(-10000000000);
     }
     (*LastRepData).push_back(temp);
     }*/
    
    //initialize the sensor map in the base station
    *SensorID_SensorPP = SensorMap;
    
    //initialize the user request map in the base station
    for (map<int, User*>::iterator it = UserMap.begin(); it != UserMap.end(); ++it) {
        
        UserID_RequestHeadP->insert(pair<int, REQUEST*>(it->first, NULL));
    }
}

void SensorNet_System::Initialize_System(int sensorNumbers, float capacity, float trans_energy, int userNumbers, float Operation_Period, float Lambda, POSITION areaStart, POSITION areaEnd, Sub_Area subArea){
    
    //for this version, capacity is ..., transmit action will need energy .., so totally it can have .. transmission;
    //and here we will assume we have ... energy level, from E0 to E...
    
    //initialize two sensors; capacity, the transmit action energy and energy harvesting rate will be used in caculation!!!!!!!!!!need to be the same as the initialization here
    //here let the capacity, transmit action energy and energy harvesting rate of the two sensors be the same, may change later!!!!!!!!!!!
    
    this->system_operationTime = Operation_Period;
    //this->myEHarvestModel.set_EnergyHarvest_Rate(e_Harvest_Rate);
    this->myTemperatureModel.set_Current_Cloudiness();
    
    this->UserNumber = userNumbers;
    this->SensorNumber = sensorNumbers;
    
    this->Area_Start = areaStart;
    this->Area_End = areaEnd;
    
    this->Subarea = subArea;
    this->Subarea_NO.rowNO = (Area_End.y - Area_Start.y) / subArea.width;
    this->Subarea_NO.columnNO = (Area_End.x - Area_Start.x) / subArea.length;
    
    //float unitX = (Area_End.x - Area_Start.x) / sensorNumbers;
    //float unitY = (Area_End.y - Area_Start.y) / 2;
    
    vector<POSITION> sensors_location(sensorNumbers);
    sensors_location[0].x = 6;
    sensors_location[0].y = 4;
    //sensors_location[1].x = 9;
    //sensors_location[1].y = 4;
    
    for (int i = 1; i <= sensorNumbers; i++) {
        int sensor_ID = i;
        Sensor* newSensorP = new Sensor;
        //float positionX = Area_Start.x+unitX/2+unitX*(i-1);
        //float positionY = Area_Start.y+unitY;
        float sensing_range = 8;
        cout << "sensor ID :" << sensor_ID << "x: " << sensors_location[i-1].x << "y:" << sensors_location[i-1].y << endl;
        //cout << " the address of myTemperatureModel is :" << &myTemperatureModel;
        Initialize_Sensor(newSensorP, sensors_location[i-1].x, sensors_location[i-1].y, sensing_range, capacity, trans_energy, &myEHarvestModel, &myTemperatureModel);
        SensorMap.insert(std::pair<int, Sensor*>(sensor_ID, newSensorP)); //add a sensor into the map
        
    }
    
    //the operation period and lambda will be used in caculation!!!!!!!!!
    for (int i = 1; i <= userNumbers; i++) {
        
        int user_ID = i;
        User* newUserP = new User;
        Initialize_User(newUserP, Operation_Period, Lambda);
        UserMap.insert(std::pair<int, User*>(user_ID, newUserP));
    }
    
    //initialize the base station, and the system only has one base station
    Initialize_BaseStation();
    
    
}

void SensorNet_System::System_Recycle(){
    
    //delete the sensors
    for (map<int, Sensor*>::iterator it = SensorMap.begin(); it != SensorMap.end(); ++it){
        
        delete it->second;
    }
    //delete the users
    for (map<int, User*>::iterator it = UserMap.begin(); it != UserMap.end(); ++it) {
        
        delete it->second;
    }
}

void Table_Caculation(User* userP, float p_refuse, SensorNet_System* mySystem){
    
    
    const bool Not_Transmit = 0;  //how about #define??? why couldn't work???!!!
    const bool Transmit = 1;
    
    //here, for the poisson process, the n is ...
    int Poisson_n = mySystem->system_operationTime / Delta_T;
    
    //float Energy_Harvesting_Rate = e_Harvest_Rate; //per second
    float Energy_Tansmit_Action = mySystem->SensorMap[1]->get_TransmitActEnergy(); //energy unit
    
    float Area_Overlap, Area_WithinSenseRange;
    Area_Overlap = M_PI * pow(5, 2) * (53 + 53) / 360 - 8 * 3 * 1 / 2;
    Area_WithinSenseRange = 8 * 6;
    
    float lambda = mySystem->UserMap[1]->User_Request_Model.get_lambda();
    
    //make a 4D vector of ranking No_TimeLevels * No_EnergyLevels * No_DataLevels * No_DataLevels
    vector<pair<float, bool>> vec(NO_DataLevels, make_pair(0.0, Not_Transmit));
    vector<vector<pair<float, bool>>> vec_2d(NO_DataLevels, vec);
    vector<vector<vector<pair<float, bool>>>> vec_3d(NO_EnergyLevels,vec_2d);
    vector<vector<vector<vector<pair<float, bool>>>>> Lookup_Table(NO_TimeLevels, vec_3d);
    
    //vector<vector<vector<pair<float, bool>>>>::iterator time_itr;
    //vector<vector<pair<float, bool>>>::iterator energy_itr;
    //vector<pair<float, bool>>::iterator delta_itr;
    
    
    //a data d_current -> d'_current transit possiblity table
    vector<float> p_vec(NO_DataLevels);
    vector<vector<float>> p_table(NO_DataLevels, p_vec);
    
    //initialize the possiblity table
    for (vector<vector<float>>::iterator it = p_table.begin(); it != p_table.end(); ++it) {
        for (vector<float>::iterator it2 = (*it).begin(); it2 != (*it).end(); ++it2) {
            
            (*it2) = 1.0 / NO_DataLevels;
            cout << *it2 << endl;
        }
    }
    
    ofstream mydatafile;
    mydatafile.open("/Users/LylaChen/Desktop/decisionTable.txt");
    
    for (int e = 0; e < NO_EnergyLevels; e++) {  // record: 1000 e d_lastreport d_current 0
        for (int d_lastreport = 0; d_lastreport < NO_DataLevels; d_lastreport++) {
            for (int d_current = 0 ; d_current < NO_DataLevels; d_current++) {
                
                mydatafile << NO_TimeLevels - 1 << " " << e << " " << d_lastreport << " " << d_current << " "<< 0 << endl;
            }
        }
    }
    
    
    //before doing the below output, i need to do the initial level output to the text first as above
    for (int t = NO_TimeLevels - 2; t >= 0; t--) {       //here t means time levels will be showed in the file 999...0
        for (int e = 0; e < NO_EnergyLevels; e++) {      //change the position of these two
            for (int d_lastreport = 0; d_lastreport < NO_DataLevels; d_lastreport++) {
                for (int d_current = 0; d_current < NO_DataLevels; d_current++) {
                    
                    TIME Calculation_Time;
                    
                    Calculation_Time.day = Simulation_Start_Time.day + (t / 60) / 24;
                    Calculation_Time.hour = Simulation_Start_Time.hour + (t / 60) % 24;
                    Calculation_Time.minute = Simulation_Start_Time.minute + t % 60;
                    
                    float Energy_Harvesting_Rate =  mySystem->myEHarvestModel.get_EnergyHarvestRate(Calculation_Time);
                    float Value_Sum_withTrans = 0.0;
                    float Value_Sum_withoutTrans = 0.0;
                    
                    if (e < Energy_Tansmit_Action/Energy_Level_Interval) {
                        //cout << "again!!!" << endl;
                        for (int p_colum = 0 ; p_colum < NO_DataLevels; p_colum++) { //p_colum means d_current -> d'_current transit possiblity table column
                            
                            //below, actually the possibility is not right, we actually may need a new table to caculate the possiblity and realize the relation to another p table!!!!!!!!!
                            
                            int future_energy_level = e+(int)(Energy_Harvesting_Rate*Delta_T/Energy_Level_Interval);
                            if (future_energy_level > (int)(mySystem->SensorMap[1]->get_capacity() / Energy_Level_Interval)) {
                                
                                Value_Sum_withoutTrans = p_table[d_current][p_colum] * Lookup_Table[t+1][(int)(mySystem->SensorMap[1]->get_capacity() / Energy_Level_Interval)][d_lastreport][p_colum].first +Value_Sum_withoutTrans;
                            }
                            else{
                                Value_Sum_withoutTrans = p_table[d_current][p_colum] * Lookup_Table[t+1][future_energy_level][d_lastreport][p_colum].first +Value_Sum_withoutTrans;
                            }
                        }
                        
                        Lookup_Table[t][e][d_lastreport][d_current].first = lambda / Poisson_n * mySystem->UserMap[1]->Value_of_Data(Temperature_Difference_Min * (d_lastreport - d_current), Temperature_Min + Temperature_Difference_Min * d_current) + Value_Sum_withoutTrans;
                        Lookup_Table[t][e][d_lastreport][d_current].second = Not_Transmit;
                        //cout << Lookup_Table[t][e][d_lastreport][d_current].second<< " ";
                        //cout << Lookup_Table[t][e][d].first << " ";
                        mydatafile << t << " " << e << " " << d_lastreport << " " << d_current << " " << Lookup_Table[t][e][d_lastreport][d_current].second << endl;
                        
                    }
                    else{
                        
                        for (int p_colum = 0 ; p_colum < NO_DataLevels; p_colum++) {
                            
                            int futureELevel_withTrans = e - (int)((Energy_Tansmit_Action - Energy_Harvesting_Rate * Delta_T) / Energy_Level_Interval);
                            if (futureELevel_withTrans > (int)(mySystem->SensorMap[1]->get_capacity() / Energy_Level_Interval)) {
                                
                                futureELevel_withTrans = (int)(mySystem->SensorMap[1]->get_capacity() /Energy_Level_Interval);
                            }
                            Value_Sum_withTrans = p_table[d_current][p_colum] * Lookup_Table[t+1][futureELevel_withTrans][d_current][p_colum].first +Value_Sum_withTrans;
                            //below, actually the possibility is not right, we actually may need a new table to caculate the possiblity and realize the relation to another p table!!!!!!!!!
                            int futureELevel_withoutTrans = e+(int)(Energy_Harvesting_Rate*Delta_T/Energy_Level_Interval);
                            if (futureELevel_withoutTrans > (int)(mySystem->SensorMap[1]->get_capacity() / Energy_Level_Interval)) {
                                
                                futureELevel_withoutTrans = (int)(mySystem->SensorMap[1]->get_capacity() / Energy_Level_Interval);
                            }
                            Value_Sum_withoutTrans = p_table[d_current][p_colum] * Lookup_Table[t+1][futureELevel_withoutTrans][d_lastreport][p_colum].first +Value_Sum_withoutTrans;
                        }
                        
                        float TotalValue_withTrans = mySystem->UserMap[1]->Value_of_Data(0.0, Temperature_Min + d_current * Temperature_Difference_Min) + Value_Sum_withTrans;
                        float TotalValue_withoutTrans =  mySystem->UserMap[1]->Value_of_Data(Temperature_Difference_Min * (d_lastreport - d_current), Temperature_Min + Temperature_Difference_Min * d_current) + Value_Sum_withoutTrans;
                        if (TotalValue_withTrans > TotalValue_withoutTrans) {
                            
                            Lookup_Table[t][e][d_lastreport][d_current].first = lambda / Poisson_n * TotalValue_withTrans + (1 - lambda / Poisson_n) * Value_Sum_withoutTrans;
                            Lookup_Table[t][e][d_lastreport][d_current].second = Transmit;
                            //cout << Lookup_Table[t][e][d_lastreport][d_current].second << " ";
                            //cout << Lookup_Table[t][e][d].first << " ";
                            
                            mydatafile << t << " " << e << " " << d_lastreport << " " << d_current << " " << Lookup_Table[t][e][d_lastreport][d_current].second << endl;
                        }
                        else{
                            Lookup_Table[t][e][d_lastreport][d_current].first = lambda / Poisson_n * TotalValue_withoutTrans + (1 - lambda / Poisson_n) * Value_Sum_withoutTrans;
                            Lookup_Table[t][e][d_lastreport][d_current].second = Not_Transmit;
                            //cout << Lookup_Table[t][e][d_lastreport][d_current].second << " ";
                            //cout << Lookup_Table[t][e][d].first << " ";
                            
                            mydatafile << t << " " << e << " " << d_lastreport << " " << d_current << " " << Lookup_Table[t][e][d_lastreport][d_current].second << endl;
                        }
                    }
                }
            }
            //cout<<endl;
        }
        //cout << endl << endl;
    }
    
    mydatafile.close();
}

void Simulation(SensorNet_System* mySystem){
    
    //float P_refuse;
    
    //After initilization, simulation starts
    Simulation_Current_Time = Simulation_Start_Time;
    
    //User generate requests, actually should be every user will generate its next request and put it into its request head in the base station
    for (int i = 1; i <= mySystem->get_UserNumber(); i++) {
        //put the request into the base station
        (*((mySystem->access_BaseStation())->access_UserID_RequestHead()))[i] = ((*mySystem->get_UserMap())[i])->generate_nextRequest();
    }
    
    int Minute_From_Start = 0;
    
    while (Minute_From_Start < mySystem->get_operationTime()/60)
    {   //a loop
        
        (mySystem->access_BaseStation())->find_firstRequest();
        
        //double check if the request time is later than the system operation time
        TIME RequestTime = (*((mySystem->access_BaseStation)()->get_FirstReq())).time;
        int RequestMinute_From_Start = RequestTime.day * 24 * 60 + RequestTime.hour * 60 + RequestTime.minute - (Simulation_Start_Time.day * 24 * 60 + Simulation_Start_Time.hour * 60 + Simulation_Start_Time.minute);
        if( RequestMinute_From_Start > mySystem->get_operationTime()/60){
            
            break;
        }
        mySystem->access_BaseStation()->Total_Request_NO++;
        //sensors will handle the request using different Algorithms
        for (int Algorithm_NO = 1; Algorithm_NO <= 3; Algorithm_NO++) {
            
            int flag_of_round = 1;
            mySystem->access_BaseStation()->LastRepDataSensorNO_Flag = 0;
            while (!(*(mySystem->access_BaseStation()->get_SensorAvailable_Algorithm(Algorithm_NO))).empty()){
                
                (mySystem->access_BaseStation())->send_Request_Algorithm(Algorithm_NO);
                //let the chosed sensor process the request
                int requested_SensorID = (mySystem->access_BaseStation)()->get_SensorID_Requested();
                if (flag_of_round == 1) { //which means this sensor is the first to be requested
                    float temp = (*(mySystem->get_SensorMap()))[requested_SensorID]->get_Total_Req_NO_Algorithm(Algorithm_NO);
                    (*(mySystem->get_SensorMap()))[requested_SensorID]->set_Total_Req_NO_Algorithm(Algorithm_NO, temp+1);
                }
                (*(mySystem->get_SensorMap()))[requested_SensorID]->process_request_Algorithm(Algorithm_NO, *((mySystem->access_BaseStation())->get_FirstReq()));
                
                
                if ((*(mySystem->get_SensorMap()))[requested_SensorID]->get_TransDecision() == 1) {
                    //sensor transmit back
                    //update the vector of last reported data in the base station and the requested sensor with the current sensed data
                    (mySystem->access_BaseStation())->update_LastRepData_Algorithm(Algorithm_NO, (*(mySystem->get_SensorMap()))[requested_SensorID]->get_Current_Sensed_Data(), requested_SensorID);
                    mySystem->access_BaseStation()->update_LastRepDataTime_Algorithm(Algorithm_NO, Simulation_Current_Time, requested_SensorID);
                    mySystem->access_BaseStation()->LastRepDataSensorNO_Flag = requested_SensorID;
                    (*(mySystem->get_SensorMap()))[requested_SensorID]->update_Last_Report_Data_Algorithm(Algorithm_NO, (*(mySystem->get_SensorMap()))[requested_SensorID]->Current_Sensed_Data);
                    
                    //cout << "Round:" << flag_of_round << " :" << "sensor ID :" <<requested_SensorID << " Decsion is : Transmit!!!" << endl;
                    break;
                    
                }
                else{ //sensor does not transmit back the data
                    //Base Station will send the request to other sensors and repeat the above
                    //delete this sensor from the available vector in the base station
                    vector<int>* temp = mySystem->access_BaseStation()->get_SensorAvailable_Algorithm(Algorithm_NO);
                    temp->erase(remove(temp->begin(),temp->end(), requested_SensorID), temp->end());
                    
                    if (flag_of_round == 1) { //which means this sensor is the first to be requested and it refuses to transmit back fresh data
                        float temp = (*(mySystem->get_SensorMap()))[requested_SensorID]->get_Total_NotTrans_NO_Algorithm(Algorithm_NO);
                        (*(mySystem->get_SensorMap()))[requested_SensorID]->set_Total_NotTrans_NO_Algorithm(Algorithm_NO, temp+1);
                    }
                    //cout << "Round:" << flag_of_round << " :" << "sensor ID :" <<requested_SensorID << " Decsion is : NOT transmit!!!" << endl;
                }
                flag_of_round++;
            }
            //until no one else the base station will just give the old data to the user and do not need to update the last reported data
            //now we can calculate the reward gained this round
            if (mySystem->access_BaseStation()->LastRepDataSensorNO_Flag == 0) { //no one transmits the fresh data back, the base station will just give the old last report data to the user, since the sensors do not transmit back the fresh data may have two reasons: no need or no energy; if no need, then either of the last report data is ok; if no energy, then the lastest last report data may be a better choice; so may be we can signal the base station when the reason is no need? Or when can always choose the latest last report data in this condition? here, we will adopt the second method!!!
                mySystem->access_BaseStation()->No_Data_TransBack_NO_Algorithm[Algorithm_NO-1]++;
                /*mySystem->access_BaseStation()->add_Actual_Action_Algorithm(Algorithm_NO, 0);*/
                float added_reward_this_round = mySystem->UserMap[mySystem->access_BaseStation()->get_FirstReq_UserID()]->Value_of_Data((*((mySystem->access_BaseStation()->access_LastRepData_Algorithm(Algorithm_NO))))[0] - mySystem->access_TemperatureModel()->Current_Temperature, mySystem->access_TemperatureModel()->Current_Temperature);
                /*mySystem->access_BaseStation()->record_added_reward_everyRound_Algorithm(Algorithm_NO, added_reward_this_round);*/
                mySystem->access_BaseStation()->add_Reward_Algorithm(Algorithm_NO, added_reward_this_round);
            }
            else{ //one of the sensors transmit the fresh data back to the base station
                /*mySystem->access_BaseStation()->add_Actual_Action_Algorithm(Algorithm_NO, 1);*/
                float added_reward_this_round = mySystem->UserMap[mySystem->access_BaseStation()->get_FirstReq_UserID()]->Value_of_Data((*(mySystem->access_BaseStation()->access_LastRepData_Algorithm(Algorithm_NO)))[mySystem->access_BaseStation()->LastRepDataSensorNO_Flag - 1] - mySystem->access_TemperatureModel()->Current_Temperature, mySystem->access_TemperatureModel()->Current_Temperature);
                /*mySystem->access_BaseStation()->record_added_reward_everyRound_Algorithm(Algorithm_NO, added_reward_this_round);*/
                mySystem->access_BaseStation()->add_Reward_Algorithm(Algorithm_NO, added_reward_this_round);
            }
        }
        
        //close the request
        //the last reported data of this requested location will not change
        //close the request and generate the next request!
        (*((mySystem->access_BaseStation())->access_UserID_RequestHead()))[mySystem->access_BaseStation()->get_FirstReq_UserID()] = (*(mySystem->get_UserMap()))[mySystem->access_BaseStation()->get_FirstReq_UserID()]->generate_nextRequest();
        mySystem->access_BaseStation()->delete_Req();
        mySystem->access_TemperatureModel()->set_Current_Temperature_Flag(0);
        
        Minute_From_Start = Simulation_Current_Time.day * 24 * 60 + Simulation_Current_Time.hour * 60 + Simulation_Current_Time.minute - (Simulation_Start_Time.day * 24 * 60 + Simulation_Start_Time.hour * 60 + Simulation_Start_Time.minute);
        
    }
    
    /*float total_notTrans_no1 = (*(mySystem->get_SensorMap()))[1]->get_Total_NotTrans_NO();
     float total_request_no1 = (*(mySystem->get_SensorMap()))[1]->get_Total_Req_NO();
     
     float total_notTrans_no2 = (*(mySystem->get_SensorMap()))[2]->get_Total_NotTrans_NO();
     float total_request_no2 = (*(mySystem->get_SensorMap()))[2]->get_Total_Req_NO();
     
     float P_Refuse1 = total_notTrans_no1 / total_request_no1;
     
     float P_Refuse2 = total_notTrans_no2 / total_request_no2;
     
     P_refuse = (P_Refuse1 + P_Refuse2) / 2;
     
     
     return P_refuse; //the probability of refusing*/
    cout << "The total reward gained using Algorithm 1 is : " << mySystem->access_BaseStation()->Reward_Gained_Algorithm[1] << endl;
    cout << "The total reward gained using Algorithm 2 is : " << mySystem->access_BaseStation()->Reward_Gained_Algorithm[2] << endl;
    cout << "The total reward gained using Algorithm 3 is : " << mySystem->access_BaseStation()->Reward_Gained_Algorithm[3] << endl;
    
    cout << "The Total Number of the User Requests to the Base Station is: " << mySystem->access_BaseStation()->Total_Request_NO << endl;
    cout << "The Number of No Fresh Data Transmitting back to the Base Station using Algorithm 1 is : " << mySystem->access_BaseStation()->No_Data_TransBack_NO_Algorithm[0] <<endl;
    cout << "The Number of No Fresh Data Transmitting back to the Base Station using Algorithm 2 is : " << mySystem->access_BaseStation()->No_Data_TransBack_NO_Algorithm[1] <<endl;
    cout << "The Number of No Fresh Data Transmitting back to the Base Station using Algorithm 3 is : " << mySystem->access_BaseStation()->No_Data_TransBack_NO_Algorithm[2] <<endl;
    
    /*ofstream ActualActionAlgorithm123("/Users/LylaChen/Desktop/ActualActionAlgorithm123SHSS.txt");
     ofstream AddedRewardEveryRoundAlgorithm123("/Users/LylaChen/Desktop/AddedRewardEveryRoundAlgorithm123SHSS.txt");
     
     for (int i = 0; i < mySystem->access_BaseStation()->Total_Request_NO; i++) {
     
     ActualActionAlgorithm123 << mySystem->access_BaseStation()->Actual_Action_Algorithm1[i] << "   "
     << mySystem->access_BaseStation()->Actual_Action_Algorithm2[i] << "   " << mySystem->access_BaseStation()->Actual_Action_Algorithm3[i] << endl;
     AddedRewardEveryRoundAlgorithm123 << mySystem->access_BaseStation()->Every_round_added_reward_Algorithm1[i] << "   " << mySystem->access_BaseStation()->Every_round_added_reward_Algorithm2[i] << "   " << mySystem->access_BaseStation()->Every_round_added_reward_Algorithm3[i] << endl;
     }*/
    
}


