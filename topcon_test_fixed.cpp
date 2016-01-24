/* 
 * File:   main.cpp
 * Author: Rafal
 *
 * Created on 22 January 2016, 3:01 PM
 */

#include <cstdlib>
#include <iostream>
#include <stdlib.h>
#include <map>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <sys/time.h>
#include <limits.h>
#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>



using namespace std;

/*
 *  Fixed map transposal implementation
 */
void transform (float **array, size_t dim) {
    float buff;
    for(int i=0; i<dim; ++i) {
        for(int j=i+1; j<dim; ++j) {
                buff = array[i][j];
                array[i][j] = array[j][i];
                array[j][i] = buff;
        }
    }
}


// sen - string containing the sentence
// out - result
// out needs to be allocated
void reverse_words_low_level(const char* sent, char *out) {
    size_t len = strlen(sent);

    bool in_the_middle = false;
    size_t word_size = 0;
    size_t curr_dest_index = 0;

    for(int i = len-1; i >= 0; --i) {
        if(sent[i] != ' ') {
            if(!in_the_middle) {
                in_the_middle = true;
                word_size=0;
            }
            word_size++;
            // if the word starts at 0 index
            if(i == 0) {
                memcpy(out + curr_dest_index, sent, word_size);
                if(curr_dest_index + word_size < len) {
                    out[curr_dest_index + word_size] = ' ';
                }
            }
        } else {
            if(in_the_middle) {
                in_the_middle = false;
                memcpy(out + curr_dest_index, sent+i+1, word_size);
                out[curr_dest_index + word_size] = ' ';
                curr_dest_index += word_size + 1;
            }
        }            
    }
}

// sen - string containing the sentence
// out - result
void reverse_words_std(const string& sent, string& out) {
    istringstream iss(sent);
    vector<string> words;
    do
    {
        string sub;
        iss >> sub;
        if(sub.size())
            words.push_back(sub);
    } while (iss);
    
    vector<string>::const_reverse_iterator it = words.rbegin();
    
    for(; it!=words.rend(); ++it) {
        out.append(*it);
        out.append(" ");
    }
} 




template <class KEY, class VALUE>
class LRUCache {   
    public:
        LRUCache(std::size_t maxItems):
            _max(maxItems),
            _timestamp(0)
            {}

            
        inline boost::optional<VALUE> Get(const KEY& key)  {
            boost::mutex::scoped_lock lock(_mutex);
            
            typename map<KEY, VALUE>::iterator it = _theMap.find(key);
            boost::optional<VALUE> result; 
            
            if(it != _theMap.end()) {
                result = it->second;
                
                // update timestamp
                _timestampMap[_timestamp++] = key;
                
                if(_timestamp == LONG_MAX ) {
                    fixTimeStamps();
                }                
            }
            
            return result;
        }
        
        
        inline void Put(const KEY &key, const VALUE &value) {
            boost::mutex::scoped_lock lock(_mutex);
            
            if(_theMap.size() >= _max) {
                typename map<KEY,VALUE>::iterator it = _theMap.find(key);
                if(it != _theMap.end()) {
                    // delete LRU
                    _theMap.erase(_timestampMap.begin()->second);
                    
                    _theMap[key] = value;
                } else {
                    _theMap[key] = value;
                }
            } else {
                _theMap[key] = value;
            }
            
            _timestampMap[_timestamp++] = key;
            
            if(_timestamp == LONG_MAX ) {
                fixTimeStamps();
            }
        }
        
        
    private:
        void fixTimeStamps(void) {
            typename std::map<long, KEY>::iterator it = _timestampMap.begin();

            for(; it!=_timestampMap.end(); ++it) {
                it->first = LONG_MAX - (it->first - _max);
            }            
        }
        
        
        std::size_t _max;
        std::map<KEY, VALUE> _theMap; 
        std::map<long, KEY> _timestampMap;
        boost::mutex _mutex;
        long _timestamp;
        
};


long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long useconds = te.tv_sec*1000000LL + te.tv_usec; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return useconds;
}


int main(int argc, char** argv) {
    const char *test = "one two three four five";
    char test_m[strlen(test)];
    const size_t looops = 100000;
    
    memset(test_m, 0, strlen(test));
    
    long long ts = current_timestamp();
    long long exec_time;
    
    for(int i=0; i<looops; i++) {
        reverse_words_low_level(test, test_m);
        //cout << test_m << endl;
    }
    exec_time = current_timestamp() - ts;
    
    cout << "Micro sec for low level: " << exec_time << endl;
    
    string sent(test);
    string reversed;
    
    ts = current_timestamp();
    for(int i=0; i<looops; i++) {
        reverse_words_std(sent, reversed);
    }
    ts = current_timestamp() - ts;
    
    cout << "Micro sec for low std: " << ts << endl;
    
    
    // that ratio is around 24 
    // the speed of std approach is 24 times slower
    cout << "Ratio of speed: " << ts/exec_time << endl;
    
    return 0;
}

