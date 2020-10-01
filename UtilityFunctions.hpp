//
// Created by Pitam Mitra on 2019-07-29.
//

#ifndef CCDDRONE_UTILITYFUNCTIONS_HPP
#define CCDDRONE_UTILITYFUNCTIONS_HPP


#include <chrono>
#include <iostream>
#include <iomanip>
#include <cmath>



std::string ColouredFmtText(std::string text, std::string colour, std::string fmt="std");
std::string ColouredFmtText(int text, std::string colour, std::string fmt="std");


class ProgressBar {
private:
    unsigned long items = 0;
    const unsigned int bar_width=50;
    const char complete_char = '=';
    const char incomplete_char = ' ';
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point start_time_frame;
    unsigned long total_items;

    int CurrentFrame=1;
    int TotalFrame=1;
    float TotalItemsInPastFrames=0;

public:
    ProgressBar (){};

    void SetEssentials(unsigned long total_items, std::chrono::system_clock::time_point sttime, int Total_Frame=1) {
        this->total_items=total_items;
        this->start_time=sttime;
        this->start_time_frame=sttime;
        this->TotalFrame=Total_Frame;
    }

    void updProgress(unsigned long new_items) { items = new_items; }

    void updProgressFrame(int Current_Frame) {

        this->CurrentFrame=Current_Frame+1;
        this->start_time_frame=std::chrono::system_clock::now();
        this->TotalItemsInPastFrames = this->items * Current_Frame;

    }



    void display() const
    {
        float progress = (float) items / (float) total_items;
        int pos = (int) (bar_width * progress);

        /*This frame*/
        auto _elapsedDurationMillis = std::chrono::duration<double, std::milli> (std::chrono::system_clock::now() - start_time_frame);
        int _elpasedMilli = _elapsedDurationMillis.count();
        float fractionRemain = 1.0-progress;
        float _estimatedTimeRemain = fractionRemain * (float)_elpasedMilli/(1000.0*progress);

        /*All Frames*/
        auto _elapsedDurationMillisAll = std::chrono::duration<double, std::milli> (std::chrono::system_clock::now() - start_time);
        int _elpasedMilliAll = _elapsedDurationMillisAll.count();
        float progressAll = (float) (TotalItemsInPastFrames+items) / (float) (total_items*TotalFrame);
        float fractionRemainAll = 1.0-progressAll;
        float _estimatedTimeRemainAll = fractionRemainAll * (float)_elpasedMilliAll/(1000.0*progressAll);


        std::cout << "[";

        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << complete_char;
            else if (i == pos) std::cout << ">";
            else std::cout << incomplete_char;
        }
        std::cout << "] ";
        std::cout << std::fixed;
        std::cout << std::setprecision(0);
        std::cout << std::setfill(' ') << std::setw(3) << progress * 100.0 << "% ";

        if (this->TotalFrame != 1){
            std::string ChunkInfoCur = std::to_string(CurrentFrame);
            ChunkInfoCur = ColouredFmtText(ChunkInfoCur, "green");
            std::string ChunkInfoTot = std::to_string(TotalFrame);
            ChunkInfoTot = ColouredFmtText(ChunkInfoTot, "red");
            std::cout<<" Frame: "<<ChunkInfoCur<<"/"<<ChunkInfoTot;
        }

        if (!std::isnan(_estimatedTimeRemain) && (progress > 0.005 ) ){

            if (this->TotalFrame != 1) std::cout<< " | Time remaining:  Frame/All " << ColouredFmtText(_estimatedTimeRemain,"green") << "/"<<ColouredFmtText(_estimatedTimeRemainAll,"red")<<" sec   ";
            else std::cout<< " | Time remaining:  " << ColouredFmtText(_estimatedTimeRemain,"green") <<" sec   ";
        } 


        std::cout<< "\r";
        std::cout.flush();
    }

    void done() const
    {
        display();
        std::cout << std::endl;
    }
};



#endif //CCDDRONE_UTILITYFUNCTIONS_HPP
