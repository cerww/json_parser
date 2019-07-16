#include "json.h"
#include <vector>
#include <algorithm>
#include <numeric>


void json_array_test() {
	const std::string json_data = R"([1,2,3,[1,2]])";
	auto r = json_parser()(json_data);
	assert(r.success());
	auto json_obj = std::move(r.value());
	assert(json_obj.size() == 4);
	assert(json_obj[3].size() == 2);
}

void json_test() {
	const std::string json_data = 
	R"( 
	{     
		"obj":{
			"obj_string":"abc",
			"abc" : null,
			"cat" :[ {"potato" :   ["1",2,3,4,5]}   ]
		},
		"potato" :[1, 2 ,3     ,4,5, [6   ,7,8,9,0  ,{   }  ],{"abc" : 134 ,"bcd":[  123 ]}],

		"hamtaro"  :{
			"bop" :[]   ,
			"boop" :  [1],
			"aaa":[{"potato":12}],
			"ccc":{"abc":[{},{},{}]}
		}
	
		
	}     
	)";

	json_parser p;
	auto result = p(json_data);
	assert(result.success());
	json_obj obj = result.value();
	assert(obj["abc"]["cat"].size() == 1);
	

	int i = 0;
}



int main() {

	json_array_test();
	json_test();
	std::cin.get();
	return 0;
}
