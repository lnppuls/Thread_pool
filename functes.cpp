#include<iostream>
#include<functional>
#include<queue>
using namespace std;


void run(){
    cout << "run once" << endl;
}


int main(){
    queue<function<void()>> mque;
    for(int i = 0;i < 10;i++){
        mque.emplace(run);
    }
    for(int i = 0;i < 10;i++){
        function<void()> this_run = mque.front();
        mque.pop();
        this_run();
    }
}