#include <iostream>
#include <cstdio>
#include "tsat_drivers.h"

using namespace std;
int main() {
    tsat_init();
    int fan;
    double volt;
    while (1) {
        cout << "Enter fan number: ";
        cin >> fan;
        cout << "Enter voltage (0-10V): ";
        cin >> volt;
        if (fan != 1 && fan != 0) {
            cout << "Invalid Fan Number (0 or 1 only)" << endl;
            continue;
        }
        printf("Commanded fan %d, %6.4f\n", fan, volt);
        tsat_write(fan, volt);
    }
    tsat_deinit();
}