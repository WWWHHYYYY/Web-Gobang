#include "webgobang.hpp"

int main()
{
    WebGobang* wg = new WebGobang();
    if(wg == NULL)
        return -1;
    
    if(wg->Init() < 0)
    {
        cout<<"init fialed"<<endl;
        return -2;
    }
    wg->StartWebGobang();
    delete wg;
    return 0;
}
