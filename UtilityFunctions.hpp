//
// Created by Pitam Mitra on 2019-07-29.
//

#ifndef CCDDRONE_UTILITYFUNCTIONS_HPP
#define CCDDRONE_UTILITYFUNCTIONS_HPP


#include <chrono>
#include <iostream>
#include <iomanip>
#include <cmath>

class ProgressBar {
private:
    unsigned int items = 0;
    const unsigned int bar_width=50;
    const char complete_char = '=';
    const char incomplete_char = ' ';
    std::chrono::system_clock::time_point start_time;
    int total_items;

public:
    ProgressBar (){};

    void SetEssentials(int total_items, std::chrono::system_clock::time_point sttime) {
        this->total_items=total_items;
        this->start_time=sttime;
    }

    void updProgress(int new_items) { items = new_items; }

    void display() const
    {
        float progress = (float) items / (float) total_items;
        int pos = (int) (bar_width * progress);

        auto _elapsedDurationMillis = std::chrono::duration<double, std::milli> (std::chrono::system_clock::now() - start_time);
        int _elpasedMilli = _elapsedDurationMillis.count();
        float fractionRemain = 1.0-progress;
        float _estimatedTimeRemain = fractionRemain * (float)_elpasedMilli/(1000.0*progress);


        std::cout << "[";

        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << complete_char;
            else if (i == pos) std::cout << ">";
            else std::cout << incomplete_char;
        }
        std::cout << "] ";
        std::cout << std::fixed;
        std::cout << std::setprecision(0);
        std::cout << progress * 100.0 << "% ";
        if (!std::isnan(_estimatedTimeRemain) ) std::cout<< " | Est. time remaining: " << _estimatedTimeRemain << " sec";
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
