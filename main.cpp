#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>

#include "measuring_time.h"
#include <condition_variable>
#include <atomic>
#include <deque>
#include <ctype.h>

using namespace std;
mutex mtx, mtx_2;
map<string, int> counting_words, final_count;
condition_variable cv;
atomic <bool> finished = {false};
deque<vector<string>> queue_of_vects;




bool diff_func(const pair<string, int> &a, const pair<string, int> &b){
    return a.second < b.second;
}

vector<pair<string, int>> toVector(map<string, int> mp) {
    map<string, int>::iterator map_iter;
    vector<pair<string, int>> words_vector;
    for (map_iter = mp.begin(); map_iter != mp.end(); map_iter++) {
        words_vector.push_back(make_pair(map_iter-> first, map_iter-> second));
    }
    return words_vector;
}

void alph_and_num_order(string f1, string f2){
    map<string, int>::iterator words_iter;
    ofstream f_in_alph_order;
    ofstream f_in_num_order;

    f_in_alph_order.open(f1);
    f_in_num_order.open(f2);


    for (words_iter = counting_words.begin(); words_iter != counting_words.end(); words_iter++) {
        f_in_alph_order << words_iter->first << "   " << words_iter->second << endl;
    }


    vector<pair<string, int>> vector_of_pairs = toVector(counting_words);
    sort(vector_of_pairs.begin(), vector_of_pairs.end(), diff_func);

    // TODO
    for (pair<string, int> item: vector_of_pairs){
        f_in_num_order << item.first<< "    " << item.second;
    }

    f_in_alph_order.close();
    f_in_num_order.close();
}

map<string, string> read_config(string filename) {
    string line;
    ifstream myfile;
    map<string, string> mp;
    myfile.open(filename);

    if (myfile.is_open())
    {
        while (getline(myfile,line))
        {
            int pos = line.find("=");
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            mp[key] = value;
        }

        myfile.close();
    }
    else {
        cout << "Error with opening the file!" << endl;
    }
    return mp;

};

template <class T>
T get_param(string key, map<string, string> myMap) {
    istringstream ss(myMap[key]);
    T val;
    ss >> val;
    return val;
}
void data_producer(const string &file_name){
    fstream f(file_name);
    if(f.is_open()){
        string line;
        int size_of_block = 25;
        vector<string> all_lines;


        while (getline(f, line)){
            int num_of_lines = 0;
            all_lines.push_back(line);
            while (size_of_block > num_of_lines){
                num_of_lines++;

            }
            {
                lock_guard<mutex> locker(mtx);
                queue_of_vects.push_back(all_lines);
            }
            cv.notify_one();
            all_lines.clear();
        }
        if (all_lines.size() != 0){
            {
                lock_guard<mutex> locker(mtx);
                queue_of_vects.push_back(all_lines);
            }

            cv.notify_one();
        }

        cv.notify_all();
        finished = true;
    }
    else{
        cerr<< "Error opening file! " << endl;
    }
}


void data_consumer(){
    while(!finished){
        unique_lock<mutex> locker(mtx);
        if(queue_of_vects.empty()){
            cv.wait(locker);
        }

        else
        {
            vector<string> data = queue_of_vects.front();
            queue_of_vects.pop_front();
            locker.unlock();
            for(size_t i=0; i < data.size(); ++i){
                string new_word="";
                transform(data[i].begin(), data[i].end(), data[i].begin(), ::tolower);
                for (size_t j=0; j < data[i].size(); ++j){
                    if (isalpha(data[i][j])){
                        new_word += data[i][j];

                    }
                }
                data[i] = new_word;
                if (!data[i].empty()){

                    lock_guard<mutex> lockGuard(mtx_2);
                    ++final_count[data[i]];
                }

            }
        }
    }
}

int main(){
    string filename = "config.txt";
    map<string, string> mp = read_config(filename);
    string infile, out_by_a, out_by_n;
    int num_of_threads;
    if (mp.size() != 0) {
        auto start_time = get_current_time_fenced();
        infile = get_param<string>("infile", mp);
        out_by_a = get_param<string>("out_by_a", mp);
        out_by_n = get_param<string>("out_by_n", mp);
        num_of_threads = get_param<int>("threads", mp);

        auto start_reading = get_current_time_fenced();
        thread thr = thread(data_producer, infile);

        auto start_thr = get_current_time_fenced();
        thread num_of_thread[num_of_threads];
        for (int i=0; i< num_of_threads; i++){
            num_of_thread[i] = thread(data_consumer);
        }
        thr.join();
        auto finish_reading = get_current_time_fenced();


        for (int j=0; j < num_of_threads; ++j){
            num_of_thread[j].join();
        }

        auto finish_thr = get_current_time_fenced();

        alph_and_num_order(out_by_a, out_by_n);

        auto final_time = get_current_time_fenced();

        chrono::duration<double, milli> total_time = final_time - start_time;
        chrono::duration<double, milli> reading_time = finish_reading - start_reading;
        chrono::duration<double, milli> analyzing_time = finish_thr - start_thr;

        cout << "Total: " << total_time.count() << " ms" << endl;
        cout << "Reading time: " << reading_time.count() << " ms" << endl;
        cout << "Analyzing: " << analyzing_time.count() << " ms" << endl;

    }


}
