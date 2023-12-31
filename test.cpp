#include <iostream>
#include "expandable_array.h"
using namespace std;
using namespace expandable_array;

int main()
{
    ExpandableArray<int> arr(3, 10);
    arr.resize(20);
    ExpandableArray<int> c = arr;

    return 0;
}
