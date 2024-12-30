#include "gtest/gtest.h"
#include<vector>
#include<string>
#include<iostream>
#include<algorithm>
using namespace std;

bool compare(vector<int>& A, vector<int>& B)
{
    if(A.size()<B.size())
    {

        return 0;
    }
    else if(A.size()>B.size())
    {
        return 1;
    }
    
    for(int i = A.size()-1; i>=0;i--)
    {
        if(A[i]<B[i])
        {
            return 0;
        }
    }
    
    return 1;

}

vector<int> sub(vector<int>& A, vector<int>& B)
{
    if(!compare(A,B))
                {
                    char m = '-';
                    cout<<m;
                return sub(B,A);}
    vector<int>C;
    int c = 0;
    int d = 0;
    for(int i = 0; i < A.size(); i++){
        c = A[i] - c;
        if(i < B.size()) {
            if(c < B[i]){
                c = c+10-B[i];
                C.push_back(c);
                c = 1;
            }
            else{
                c = c-B[i];
                C.push_back(c);
                c = 0;
            }
        }else{
            if(c<0){
                C.push_back(c+10);
                c = 1;
            }else{
                C.push_back(c);
                c = 0;
            }
        }
    }
    
    if(C.back()!=0)return C;
    
    for(int i = C.size()-1; i > 0;i--)
    {
        if(C[i]!=0)break;
        C.pop_back();
    }
    return C;
}

TEST(Test, Test1)
{
    string a,b;
    cin>>a>>b;
    vector<int>A,B;
    for(int i = a.size()-1; i >=0; i--){
        A.push_back(a[i]-'0');
    }

    for(int i = b.size()-1; i >=0; i--){
        B.push_back(b[i]-'0');
    }

    auto C = sub(A, B);
    for(int i = C.size()-1; i >=0;i--){
        cout<<C[i];
    }
}
