#ifndef _MY_STRINGIFY_H_
#define _MY_STRINGIFY_H_

/* doing two levels of stringification allows us to stringify a macro */
#define my_stringify_l(x...)	#x
#define my_stringify(x...)	my_stringify_l(x)

#endif /* _MY_STRINGIFY_H_ */
