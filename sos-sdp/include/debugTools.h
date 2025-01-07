//
// Created by sergey on 06.06.23.
//

#ifndef MYPROJECT_DEBUGTOOLS_H
#define MYPROJECT_DEBUGTOOLS_H

#define debug_disabled 1

#define debugStream \
    if (debug_disabled) {} \
    else std::cout


#endif //MYPROJECT_DEBUGTOOLS_H
