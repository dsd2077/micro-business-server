#ifndef __WD_TASK_H__
#define __WD_TASK_H__
#include <functional>

namespace wd
{
using Task = std::function<void()>;
}//end of namespace wd


#endif
