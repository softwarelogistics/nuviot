#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include <Arduino.h>
#include "Console.h"

class MedianFilter {
    double *m_buffer;
    uint8_t m_size;
    uint8_t m_throwAway;
    uint8_t m_slot = 0;
    uint8_t m_populated = 0;

    
    private:

    void bubbleSort(double arr[], int arraySize)
    {
        int i, j;
        for (i = 0; i < arraySize - 1; i++)
            for (j = 0; j < arraySize - i - 1; j++)
                if (arr[j] > arr[j + 1]){
                    double temp = arr[j];
                    arr[j] = arr[j+1];
                    arr[j+1] = temp;
                }
    }


    public:
      MedianFilter(uint8_t size, uint8_t throwAway) {
        m_size = size;
        m_throwAway = throwAway;

        m_buffer = new double[m_size];
      }

      double apply(double value) {
        m_buffer[m_slot++] = value;
        if(m_slot == m_size) {
            m_populated = true;
            m_slot = 0;
        }

        if(m_populated) {
            bubbleSort(m_buffer, m_size);
            double sum = 0;
            for(int idx = m_throwAway; idx < (m_size - m_throwAway); ++idx) {
                sum += m_buffer[idx];
            }

            return sum / (double)(m_size - (m_throwAway * 2));
        }
        else {
            return value;
        }
      }

      void debug(Console *console) {
        if(m_populated) {
            double sum = 0;
            for(int idx = 0; idx < m_size; ++idx) {
                if(idx >= m_throwAway && idx < (m_size - m_throwAway)) {
                    sum += m_buffer[idx];
                }

                double avg = sum / (double)(m_size - (m_throwAway * 2));

                console->println(String(idx)  + ". VALUE: " + String(m_buffer[idx]) + " SORTED: " + String(m_buffer[idx]) + " SUM: " + String(sum) + " AVG: " + String(avg));
            }
        }
        else {
            console->println("not populated.");
        }
      }

      bool getIsPopulated() {
        return m_populated;
      }
};

#endif