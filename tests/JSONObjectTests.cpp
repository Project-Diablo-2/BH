#include "doctest/doctest.h"

#include "JSONObject.h"
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>

// BH's JSON object model (BH/JSONObject.h) backs the stash export: the "json" export type is
// JSONObject/JSONArray serialized with SER_OPT_FORMATTED, and the Mustache export types read the
// same tree through JSONElement::find paths.

namespace {
	std::string Compact(JSONElement &e) {
		std::string buf;
		JSONWriter w(buf, SER_OPT_NONE);
		e.serialize(w);
		return buf;
	}

	std::string Formatted(JSONElement &e) {
		std::string buf;
		JSONWriter w(buf, SER_OPT_FORMATTED);
		e.serialize(w);
		return buf;
	}

	JSONObject *Obj(const std::string &key, const std::string &value) {
		JSONObject *o = new JSONObject();
		o->set(key, value);
		return o;
	}
}

TEST_SUITE("JSONObject") {

TEST_CASE("Json_Escape escapes quotes, backslashes, solidus and the short control escapes") {
	CHECK(Json_Escape("plain text 123") == "plain text 123");
	CHECK(Json_Escape("Tal Rasha's \"Horadric\" Crest") == "Tal Rasha's \\\"Horadric\\\" Crest");
	CHECK(Json_Escape("C:\\d2\\save") == "C:\\\\d2\\\\save");
	CHECK(Json_Escape("a/b") == "a\\/b");
	CHECK(Json_Escape("l1\nl2\r\tx\by\fz") == "l1\\nl2\\r\\tx\\by\\fz");
	CHECK(Json_Escape("") == "");
	// Non-ASCII bytes (D2's color code byte, UTF-8 text) are valid in JSON strings and pass through.
	CHECK(Json_Escape("\xFF" "c4Shako") == "\xFF" "c4Shako");
}

TEST_CASE("Json_Escape output of C0 control characters is a valid JSON string" * doctest::should_fail()) {
	// BUG: RFC 8259 section 7 requires every character U+0000..U+001F to be escaped inside a JSON
	// string; Json_Escape only handles \b \f \n \r \t and copies e.g. 0x01 / 0x1F raw, producing
	// invalid JSON. Checked by parsing with a conforming parser, so any valid escape spelling passes.
	const std::string inputs[] = { std::string("a\x01" "b"), std::string("\x1F") };
	for (int i = 0; i < 2; i++) {
		nlohmann::json parsed = nlohmann::json::parse("\"" + Json_Escape(inputs[i]) + "\"", nullptr, false);
		REQUIRE_FALSE(parsed.is_discarded());
		CHECK(parsed.get<std::string>() == inputs[i]);
	}
}

TEST_CASE("JSONObject stores typed values and the typed getters convert between them") {
	JSONObject o;
	o.set("name", std::string("Harlequin Crest"));
	o.set("iLevel", 87);
	o.set("speed", 1.5f);
	o.set("isEthereal", true);

	CHECK(o.length() == 4);
	CHECK(o.get("name")->getType() == JSON_STRING);
	CHECK(o.get("iLevel")->getType() == JSON_NUMBER);
	CHECK(o.get("speed")->getType() == JSON_NUMBER);
	CHECK(o.get("isEthereal")->getType() == JSON_BOOL);

	CHECK(o.getString("name") == "Harlequin Crest");
	CHECK(o.getString("iLevel") == "87");
	CHECK(o.getString("isEthereal") == "true");
	CHECK(o.getNumber("iLevel") == 87.0f);
	CHECK(o.getNumber("speed") == 1.5f);
	CHECK(o.getNumber("isEthereal") == 1.0f);
	CHECK(o.getBool("isEthereal"));
	CHECK(o.get("iLevel")->toInt() == 87);
}

TEST_CASE("JSONObject getters return neutral values for missing keys and wrong container types") {
	JSONObject o;
	o.set("name", std::string("Shako"));
	o.set("child", Obj("k", "v"));
	JSONArray *arr = new JSONArray();
	arr->add(1);
	o.set("list", arr);

	CHECK(o.getString("missing") == "");
	CHECK(o.getNumber("missing") == 0.0f);
	CHECK_FALSE(o.getBool("missing"));
	CHECK(o.getObject("missing") == nullptr);
	CHECK(o.getArray("missing") == nullptr);

	// get() of a missing key is the shared Null element, never a null pointer.
	JSONElement *missing = o.get("missing");
	REQUIRE(missing != nullptr);
	CHECK(missing->getType() == JSON_NULL);
	CHECK_FALSE(missing->hasValue());

	CHECK(o.getObject("list") == nullptr);
	CHECK(o.getArray("child") == nullptr);
	CHECK(o.getObject("name") == nullptr);
	CHECK(o.getObject("child") != nullptr);
	CHECK(o.getArray("list") == arr);
}

TEST_CASE("JSONObject::set replaces an existing key and ignores empty keys and null children") {
	JSONObject o;
	o.set("a", 1);
	o.set("a", std::string("replaced"));
	CHECK(o.length() == 1);
	CHECK(o.get("a")->getType() == JSON_STRING);
	CHECK(o.getString("a") == "replaced");

	o.set("", std::string("ignored"));
	o.set("nullObj", (JSONObject*)nullptr);
	o.set("nullArr", (JSONArray*)nullptr);
	CHECK(o.length() == 1);
	CHECK(o.get("nullObj")->getType() == JSON_NULL);
}

TEST_CASE("JSONObject::get(\"this\") refers to the object itself") {
	JSONObject o;
	o.set("x", 1);
	CHECK(o.get("this") == &o);
	CHECK(o.find("this") == &o);
	CHECK(o.find("") == &o);
}

TEST_CASE("Truthiness: zero, false, empty strings and empty containers have no value") {
	CHECK_FALSE(JSONNumber(0).hasValue());
	CHECK_FALSE(JSONNumber(0.0f).hasValue());
	CHECK(JSONNumber(-1).hasValue());
	CHECK(JSONNumber(0.25f).hasValue());
	CHECK_FALSE(JSONBool(false).hasValue());
	CHECK(JSONBool(true).hasValue());
	CHECK_FALSE(JSONString("").hasValue());
	CHECK(JSONString("false").hasValue());
	CHECK_FALSE(JSONObject().hasValue());
	CHECK_FALSE(JSONArray().hasValue());

	JSONArray a;
	a.add(0);
	CHECK(a.hasValue());
	CHECK(static_cast<bool>(JSONString("x")));
	// (JSONElement::Null() is defined inline in JSONObject.cpp, so the Null element is reached via get())
	JSONObject empty;
	CHECK_FALSE(static_cast<bool>(*empty.get("missing")));
}

TEST_CASE("JSONNumber conversions: ints and floats convert to each other with C semantics") {
	JSONNumber i(42);
	CHECK(i.toInt() == 42);
	CHECK(i.toFloat() == 42.0f);
	CHECK(i.getValue() == 42.0f);
	CHECK(i.toString() == "42");
	CHECK(JSONNumber(-7).toString() == "-7");

	JSONNumber f(2.75f);
	CHECK(f.toInt() == 2); // truncation toward zero like a C cast
	CHECK(JSONNumber(-2.75f).toInt() == -2);
	CHECK(f.toFloat() == 2.75f);
	CHECK(f.getValue() == 2.75f);
	// float text must round-trip to the same value
	CHECK(atof(f.toString().c_str()) == doctest::Approx(2.75));
	CHECK(atof(JSONNumber(-0.125f).toString().c_str()) == doctest::Approx(-0.125));
}

TEST_CASE("JSONString converts to numbers like atoi/atof and to bool only for \"true\"") {
	CHECK(JSONString("42").toInt() == 42);
	CHECK(JSONString("42abc").toInt() == 42);
	CHECK(JSONString("abc").toInt() == 0);
	CHECK(JSONString("-3").toInt() == -3);
	CHECK(JSONString("12.5").toFloat() == 12.5f);
	CHECK(JSONString("true").toBool());
	CHECK_FALSE(JSONString("false").toBool());
	CHECK_FALSE(JSONString("1").toBool());
	CHECK_FALSE(JSONString("TRUE").toBool());
	CHECK(JSONBool(true).toString() == "true");
	CHECK(JSONBool(false).toString() == "false");
	CHECK(JSONBool(true).toInt() == 1);
	CHECK(JSONBool(false).toFloat() == 0.0f);
}

TEST_CASE("JSONArray stores elements in insertion order with bounds-checked access") {
	JSONArray a;
	a.add(std::string("Tal"));
	a.add(7);
	a.add(true);
	a.add(0.5f);
	a.add(Obj("name", "Ral"));
	JSONArray *inner = new JSONArray();
	inner->add(1);
	a.add(inner);
	a.add((JSONObject*)nullptr);
	a.add((JSONArray*)nullptr);

	CHECK(a.length() == 6);
	CHECK(a.getString(0) == "Tal");
	CHECK(a.getNumber(1) == 7.0f);
	CHECK(a.getBool(2));
	CHECK(a.getNumber(3) == 0.5f);
	REQUIRE(a.getObject(4) != nullptr);
	CHECK(a.getObject(4)->getString("name") == "Ral");
	CHECK(a.getArray(5) == inner);
	CHECK(a.getObject(5) == nullptr);
	CHECK(a.getArray(4) == nullptr);

	// index == length is the first out-of-range index
	CHECK(a.get(6)->getType() == JSON_NULL);
	CHECK(a.getString(6) == "");
	CHECK(a.getNumber(6) == 0.0f);
	CHECK_FALSE(a.getBool(6));
	CHECK(a.getObject(6) == nullptr);
	CHECK(a.getArray(6) == nullptr);
}

TEST_CASE("JSONArray::removeWhere and remove drop exactly the selected elements") {
	JSONArray a;
	for (int i = 1; i <= 6; i++) {
		a.add(i);
	}
	a.removeWhere([](JSONElement *e) { return e->toInt() % 2 == 0; });
	REQUIRE(a.length() == 3);
	CHECK(a.getNumber(0) == 1.0f);
	CHECK(a.getNumber(1) == 3.0f);
	CHECK(a.getNumber(2) == 5.0f);

	a.remove(a.begin());
	REQUIRE(a.length() == 2);
	CHECK(a.getNumber(0) == 3.0f);

	a.removeWhere([](JSONElement *) { return true; });
	CHECK(a.length() == 0);
}

TEST_CASE("equals compares by value: numbers numerically, objects ignoring key order, arrays in order") {
	JSONNumber twoF(2.0f);
	CHECK(JSONNumber(2).equals(&twoF));
	JSONNumber two(2);
	JSONString twoStr("2");
	JSONBool yes(true);
	CHECK_FALSE(two.equals(&twoStr));
	CHECK_FALSE(twoStr.equals(&two));
	CHECK_FALSE(yes.equals(&two));
	CHECK_FALSE(two.equals(nullptr));
	JSONObject empty;
	CHECK(empty.get("missing")->equals(nullptr));

	JSONObject a, b, c;
	a.set("x", 1);
	a.set("y", std::string("s"));
	b.set("y", std::string("s"));
	b.set("x", 1);
	c.set("x", 1);
	c.set("y", std::string("t"));
	CHECK(a.equals(&b));
	CHECK(b.equals(&a));
	CHECK_FALSE(a.equals(&c));

	JSONObject d;
	d.set("x", 1);
	CHECK_FALSE(a.equals(&d));
	CHECK_FALSE(d.equals(&a));

	JSONArray l1, l2, l3;
	l1.add(1); l1.add(2);
	l2.add(1); l2.add(2);
	l3.add(2); l3.add(1);
	CHECK(l1.equals(&l2));
	CHECK_FALSE(l1.equals(&l3));
	l2.add(3);
	CHECK_FALSE(l1.equals(&l2));
	CHECK_FALSE(l1.equals(&a));
}

TEST_CASE("JSONArray::contains returns the stored element deep-equal to the target") {
	JSONArray a;
	a.add(Obj("code", "r01"));
	a.add(Obj("code", "r02"));

	JSONObject *target = Obj("code", "r02");
	JSONElement *found = a.contains(target);
	CHECK(found == a.get(1));
	CHECK(found != target);
	delete target;

	JSONObject *absent = Obj("code", "r33");
	CHECK(a.contains(absent) == nullptr);
	delete absent;
	CHECK(a.contains(nullptr) == nullptr);
}

TEST_CASE("find resolves dotted and bracketed paths through objects and arrays") {
	JSONObject root;
	JSONObject *item = new JSONObject();
	item->set("name", std::string("Spirit"));
	JSONArray *stats = new JSONArray();
	stats->add(Obj("name", "to All Skills"));
	stats->add(Obj("name", "Faster Cast Rate"));
	item->set("stats", stats);
	root.set("item", item);
	root.set("my key", std::string("spaced"));
	root.set("a.b", std::string("dotted key"));

	CHECK(root.find("item.name")->toString() == "Spirit");
	CHECK(root.find("item.stats.1.name")->toString() == "Faster Cast Rate");
	CHECK(root.find("item.stats[0].name")->toString() == "to All Skills");
	CHECK(root.find("item.stats[1]")->getType() == JSON_OBJECT);
	CHECK(root.find("item[\"name\"]")->toString() == "Spirit");
	CHECK(root.find("item['name']")->toString() == "Spirit");
	CHECK(root.find("[\"my key\"]")->toString() == "spaced");
	CHECK(root.find("[\"a.b\"]")->toString() == "dotted key");
	CHECK(stats->find("0.name")->toString() == "to All Skills");
	CHECK(stats->find("[1].name")->toString() == "Faster Cast Rate");
	CHECK(stats->find("1")->getType() == JSON_OBJECT);
	CHECK(stats->find("this") == stats);
	CHECK(root.find("item.this") == item);
}

TEST_CASE("find of a missing path segment yields the Null element instead of crashing") {
	JSONObject root;
	JSONArray *stats = new JSONArray();
	stats->add(Obj("name", "x"));
	root.set("stats", stats);
	root.set("name", std::string("Shako"));

	CHECK(root.find("missing")->getType() == JSON_NULL);
	CHECK(root.find("missing.deeper.still")->getType() == JSON_NULL);
	CHECK(root.find("stats.1.name")->getType() == JSON_NULL); // index == length
	CHECK(root.find("stats[5]")->getType() == JSON_NULL);
	CHECK(root.find("stats.name")->getType() == JSON_NULL); // arrays have no named members
	CHECK(root.find("name.length")->getType() == JSON_NULL); // scalars have no members
	CHECK(stats->find("-1")->getType() == JSON_NULL);
}

TEST_CASE("Compact serialization writes sorted keys and skips value-less members") {
	JSONObject o;
	o.set("type", std::string("Shako"));
	o.set("iLevel", 87);
	o.set("isEthereal", true);
	o.set("sockets", 0);
	o.set("unidentified", false);
	o.set("name", std::string(""));
	o.set("socketed", new JSONArray());
	CHECK(Compact(o) == "{\"iLevel\": 87,\"isEthereal\": true,\"type\": \"Shako\"}");

	JSONObject quoted;
	quoted.set("name", std::string("say \"hi\"\n"));
	CHECK(Compact(quoted) == "{\"name\": \"say \\\"hi\\\"\\n\"}");

	JSONArray a;
	a.add(1);
	a.add(0);
	a.add(std::string("x"));
	a.add(Obj("k", "v"));
	CHECK(Compact(a) == "[1,\"x\",{\"k\": \"v\"}]");

	JSONObject neg;
	neg.set("v", -5);
	CHECK(Compact(neg) == "{\"v\": -5}");
}

TEST_CASE("Serializing an element without a value writes nothing and reports false") {
	std::string buf;
	JSONWriter w(buf, SER_OPT_NONE);
	JSONObject empty;
	JSONArray emptyArr;
	JSONString emptyStr("");
	JSONNumber zero(0);
	JSONBool no(false);
	CHECK_FALSE(empty.serialize(w));
	CHECK_FALSE(emptyArr.serialize(w));
	CHECK_FALSE(emptyStr.serialize(w));
	CHECK_FALSE(zero.serialize(w));
	CHECK_FALSE(no.serialize(w));
	CHECK(buf == "");

	// An object whose members all lack values still has keys, so it is written as {}.
	JSONObject onlyFalsy;
	onlyFalsy.set("a", 0);
	CHECK(Compact(onlyFalsy) == "{}");
}

TEST_CASE("Formatted serialization indents nested objects by two spaces per level") {
	JSONObject o;
	o.set("a", 1);
	JSONObject *c = new JSONObject();
	c->set("d", std::string("x"));
	o.set("c", c);
	o.set("b", true);
	CHECK(Formatted(o) ==
		"{\n"
		"  \"a\": 1,\n"
		"  \"b\": true,\n"
		"  \"c\": {\n"
		"    \"d\": \"x\"\n"
		"  }\n"
		"}");
}

TEST_CASE("Formatted serialization of the stash export shape puts each object on its own indented block") {
	// StashExport writes an array of item objects, each with a nested array of stat objects.
	JSONArray data;
	JSONObject *item = new JSONObject();
	item->set("type", std::string("Shako"));
	JSONArray *stats = new JSONArray();
	JSONObject *stat = new JSONObject();
	stat->set("value", 2);
	stats->add(stat);
	item->set("stats", stats);
	data.add(item);
	data.add(Obj("type", "Jewel"));

	CHECK(Formatted(data) ==
		"[\n"
		"  {\n"
		"    \"stats\": [\n"
		"      {\n"
		"        \"value\": 2\n"
		"      }\n"
		"    ],\n"
		"    \"type\": \"Shako\"\n"
		"  },\n"
		"  {\n"
		"    \"type\": \"Jewel\"\n"
		"  }\n"
		"]");
}

}
