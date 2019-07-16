#include "parsing_stuff.h"
#include <vector>
#include <unordered_map>
#include <charconv>
#include <cassert>
#include <iostream>

struct json_obj {
	struct null {};

	json_obj() = default;
	json_obj(bool b) :m_self(b) {}
	json_obj(std::string_view s) :m_self(std::string(s)) {};
	json_obj(std::vector<json_obj> s) :m_self(std::move(s)) {};

	template<typename int_, std::enable_if_t<std::is_integral_v<int_>, int> = 0>
	json_obj(int_ a) :m_self(std::in_place_type_t<int64_t>{}, a) {}

	template<typename d, std::enable_if_t<std::is_floating_point_v<d>, int> = 0>
	json_obj(d a) : m_self(std::in_place_type_t<long double>{}, a) {}

	json_obj(std::unordered_map<std::string, json_obj> m) :m_self(std::move(m)) {};

	json_obj& operator=(std::unordered_map<std::string, json_obj> m) {
		m_self = std::move(m);
		return *this;
	}

	json_obj& operator=(std::vector<json_obj> m) {
		m_self = std::move(m);
		return *this;
	}
	template<typename d, std::enable_if_t<std::is_floating_point_v<d>, int> = 0>
	json_obj & operator=(d m) {
		m_self.emplace<long double>(m);
		return *this;
	}

	template<typename int_t, std::enable_if_t<std::is_integral_v<int_t>, int> = 0>
	json_obj & operator=(int_t m) {
		m_self.emplace<int64_t>(m);
		return *this;
	}

	json_obj& operator=(std::string_view m) {
		m_self = std::string(m);
		return *this;
	}

	bool is_array()const noexcept{
		return std::holds_alternative<std::vector<json_obj>>(m_self);
	}

	bool is_string()const noexcept {
		return std::holds_alternative<std::string>(m_self);
	}

	bool is_object()const noexcept {
		return std::holds_alternative<std::unordered_map<std::string, json_obj>>(m_self);
	}

	bool is_null()const noexcept {
		return std::holds_alternative<null>(m_self);
	}

	bool is_number()const noexcept {
		return std::holds_alternative<int64_t>(m_self) || 
			std::holds_alternative<long double>(m_self);
	}

	json_obj& operator[](std::string s) {
		assert(is_object());
		auto& map = std::get<std::unordered_map<std::string, json_obj>>(m_self);
		return map[std::move(s)];
	}

	json_obj& operator[](int idx) {
		assert(is_array());
		auto& array = std::get<std::vector<json_obj>>(m_self);
		return array[idx];
	}

	size_t size()const noexcept {
		assert(is_array() || is_object() || is_string());
		if(m_self.index() == 0) {
			return std::get<0>(m_self).size();
		}else if(m_self.index() == 1) {
			return std::get<1>(m_self).size();
		}else if (m_self.index() == 5) {
			return std::get<5>(m_self).size();
		}

		if (m_self.index() != 4)
			return 1;
		else 
			return 0;//null
	}

	void push_back(json_obj new_obj) {
		assert(is_array());
		std::get<1>(m_self).push_back(std::move(new_obj));
	}

	auto& emplace_back(json_obj new_obj) {
		assert(is_array());
		return std::get<1>(m_self).emplace_back(std::move(new_obj));
	}

	void insert(std::pair<std::string,json_obj> new_obj) {
		std::get<5>(m_self).insert(std::move(new_obj));		
	}

	auto& as_array() {
		return std::get<1>(m_self);
	}

	const auto& as_array() const{
		return std::get<1>(m_self);
	}

private:
	std::variant<std::string, std::vector<json_obj>, int64_t, long double, null, std::unordered_map<std::string, json_obj>,bool> m_self = null{};
};

constexpr bool is_number(char c) noexcept {
	return c >= '0' && c <= '9';
}

struct json_string_parser {
	parse_result<json_obj> operator()(const std::string_view s) noexcept {
		if (s.empty() || (s[0] != '\'' && s[0] != '"'))
			return parse_fail();

		const char expected_quote = s[0];
		const auto last_char_it = std::adjacent_find(s.begin(), s.end(), [&](auto a, auto b) {
			return b == expected_quote && a != '\\';
		});
		//it should point to the last thing that's in the json string
		if (last_char_it == s.end())
			return parse_fail();

		const size_t size = std::distance(s.begin(), last_char_it);

		//+1 skipes the first " or ', +2 skips both
		return parse_result(json_obj(s.substr(1, size)), s.substr(size + 2));
	}
};

struct json_bool_parser{
	parse_result<json_obj> operator()(std::string_view s) const{
		if(s.size()<4) {
			return parse_fail();
		}else if (auto true_result = str_parser("true")(s);  true_result.success()) {
			return parse_result(json_obj(true), true_result.rest());
		}else if(auto false_result = str_parser("false")(s);false_result.success()) {
			return parse_result(json_obj(false), false_result.rest());
		}else {
			return parse_fail();
		}
	}
};

struct json_int_parser {
	parse_result<json_obj> operator()(std::string_view s) const {
		if (s.empty())
			return parse_fail();

		int64_t r = 0;
		const auto t = std::from_chars(s.data(), s.data() + s.size(), r);

		if (t.ptr == s.data())//nothing parsed, no int here
			return parse_fail();

		s.remove_prefix(t.ptr - s.data());

		return parse_result(json_obj(r), s);
	};
};

struct json_double_parser {
	parse_result<json_obj> operator()(std::string_view s) const {
		if (s.empty())
			return parse_fail();

		long double r = 0;
		const auto t = std::from_chars(s.data(), s.data() + s.size(), r);

		if (t.ptr == s.data())//nothing parsed, no double here
			return parse_fail();

		s.remove_prefix(t.ptr - s.data());

		return parse_result(json_obj(r), s);
	};
};

struct json_null_parser {
	parse_result<json_obj> operator()(std::string_view s) const {
		if (s.size() < 4)
			return parse_fail();

		const auto first_four = s.substr(0, 4);

		if (first_four[0] == 'n' || first_four[0] == 'N' &&
			first_four[1] == 'u' || first_four[1] == 'U' &&
			first_four[2] == 'l' || first_four[2] == 'L' &&
			first_four[3] == 'l' || first_four[2] == 'L')
		{
			s.remove_prefix(4);
			return parse_result(json_obj(), s);
		}
		return parse_fail();
	}
};

struct json_array_parser {
	parse_result<json_obj> operator()(std::string_view s)const;
};

struct json_obj_parser {
	parse_result<json_obj> operator()(std::string_view s) const {
		if (s.size() < 2 || s[0] != '{')
			return parse_fail();

		s.remove_prefix(1);//remove the '{'

		std::unordered_map<std::string, json_obj> members;

		s = whitespace_parser(s).rest();

		if (s.empty())
			return parse_fail();
		else if (s[0] == '}')
			return parse_result(json_obj(std::move(members)), s.substr(1));


		while (true) {
			auto blargland = parse_multi_consecutive(s,
				quote_string_parser{},
				whitespace_parser,
				char_parser<true>(':'),
				whitespace_parser,
				multi_parser(json_string_parser{}, json_obj_parser{}, json_double_parser{}, json_int_parser{}, json_null_parser{}, json_array_parser{}),
				whitespace_parser
			);

			if (!blargland)
				return parse_fail(blargland);
			auto& [name, _, __, ___, obj, ____] = blargland.value();
			s = blargland.rest();

			members[std::string(name)] = std::move(obj);


			if (s.empty()) {
				return parse_fail("couldn't match {");
			}else if (s[0] == '}') {
				s.remove_prefix(1);//remove the '}'
				break;
			}else if (s[0] == ',') {
				s.remove_prefix(1);
				s = whitespace_parser(s).rest();
				continue;
			}

			return parse_fail("unexpected char");
		}		

		return parse_result(json_obj(std::move(members)), s);
	}
};

struct json_parser {
	parse_result<json_obj> operator()(const std::string_view s) const {
		auto t = whitespace_parser(s).rest();
		return try_parse_multiple(t, json_obj_parser{}, json_string_parser{}, json_int_parser{},  json_double_parser{}, json_null_parser{}, json_array_parser{});
	}
};

parse_result<json_obj> json_array_parser::operator()(std::string_view s)const {
	if (s.size() < 2 || s[0] != '[')
		return parse_fail();

	s.remove_prefix(1);

	std::vector<json_obj> elems;

	s = whitespace_parser(s).rest();

	if (s.empty() || s[0] == ']')
		return parse_result(json_obj(std::move(elems)), s.substr(1));

	while (true) {
		auto parse_result = parse_multi_consecutive(s, json_parser(), whitespace_parser);

		if (!parse_result)
			return parse_fail();
		auto& [obj, _] = parse_result.value();

		s = parse_result.rest();

		elems.push_back(std::move(obj));

		if (s.empty()) {
			return parse_fail("couldn't match ]");
		}else if (s[0] == ',') {
			s.remove_prefix(1);
		}else if (s[0] == ']') {
			break;
		}else {
			return parse_fail();
		}
	}
	s.remove_prefix(1);
	return parse_result(json_obj(std::move(elems)), s);
}
