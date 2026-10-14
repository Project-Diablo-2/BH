#include "doctest/doctest.h"

#include "BH.h"
#include "JSONObject.h"
#include "Mustache.h"

#include <functional>
#include <map>
#include <memory>
#include <string>

// BH's Mustache implementation (BH/Mustache.cpp) renders the stash export "text" formats.
// Spec: https://mustache.github.io/mustache.5.html. BH extensions exercised here:
//  - section comparisons {{#key=V}} {{#key!V}} {{#key>N}} {{#key<N}} {{#key$A|B}} {{#key^A|B}}
//  - isolated partials {{>>name}} (render with only the current element, no parent lookup)
//  - {{this}} for the current element; literal "\n", "\t", "\r" escapes in template text
//  - output is Markdown text, so values are inserted verbatim (no HTML escaping)

namespace {
	typedef std::function<Mustache::AMustacheTemplate*(std::string)> Factory;

	class Partials {
	public:
		std::map<std::string, std::unique_ptr<Mustache::AMustacheTemplate>> templates;

		void Add(const std::string &name, const std::string &source) {
			Mustache::AMustacheTemplate *t = Mustache::parse(source);
			REQUIRE(t != nullptr);
			templates[name].reset(t);
		}

		Factory MakeFactory() {
			return [this](std::string name) -> Mustache::AMustacheTemplate* {
				auto it = templates.find(name);
				return it == templates.end() ? nullptr : it->second.get();
			};
		}
	};

	std::string Render(const std::string &templ, JSONElement *data, Partials *partials = nullptr) {
		Mustache::Context ctx(data, partials ? partials->MakeFactory() : Factory(nullptr));
		std::unique_ptr<Mustache::AMustacheTemplate> t(Mustache::parse(templ));
		REQUIRE(t != nullptr);
		return Mustache::renderTemplate(t.get(), ctx);
	}

	JSONObject *Obj(const std::string &key, const std::string &value) {
		JSONObject *o = new JSONObject();
		o->set(key, value);
		return o;
	}

	JSONObject *StatEntry(int value, const std::string &name, const std::string &skill = "") {
		JSONObject *s = new JSONObject();
		s->set("value", value);
		s->set("name", name);
		if (!skill.empty()) {
			s->set("skill", skill);
		}
		return s;
	}

	JSONObject *RangedStat(int value, const std::string &name, int min, int max) {
		JSONObject *s = StatEntry(value, name);
		JSONObject *range = new JSONObject();
		range->set("min", min);
		range->set("max", max);
		s->set("range", range);
		return s;
	}
}

TEST_SUITE("Mustache") {

TEST_CASE("Variables render strings, numbers and booleans; missing keys and objects render empty") {
	JSONObject o;
	o.set("name", std::string("Shako"));
	o.set("iLevel", 87);
	o.set("isEthereal", true);
	o.set("range", Obj("min", "1"));
	CHECK(Render("{{name}} (L{{iLevel}}) eth={{isEthereal}} [{{missing}}]{{range}}", &o) ==
		"Shako (L87) eth=true []");
	CHECK(Render("{{iLevel}}{{iLevel}}", &o) == "8787");
}

TEST_CASE("Variable values are inserted verbatim because the export is Markdown text, not HTML") {
	JSONObject o;
	o.set("name", std::string("Tal Rasha's <Horadric> & \"Crest\""));
	CHECK(Render("{{name}}", &o) == "Tal Rasha's <Horadric> & \"Crest\"");
}

TEST_CASE("Text without complete tags passes through unchanged") {
	JSONObject o;
	o.set("b", std::string("B"));
	CHECK(Render("", &o) == "");
	CHECK(Render("no tags here", &o) == "no tags here");
	CHECK(Render("a {{b", &o) == "a {{b");
	CHECK(Render("{{b}} }} {", &o) == "B }} {");
}

TEST_CASE("Comments are removed from the output") {
	JSONObject o;
	o.set("name", std::string("x"));
	CHECK(Render("a{{! this is ignored }}b", &o) == "ab");
	CHECK(Render("{{!name}}{{name}}", &o) == "x");
}

TEST_CASE("Backslash escapes \\n \\t \\r written in template text become control characters") {
	// BH also translates the two-character sequences \n, \t, \r in template text (legacy from the
	// line-based config format, which users' templates may still rely on).
	JSONObject o;
	o.set("v", std::string("keep\\n"));
	CHECK(Render("line1\\nline2\\tx\\r", &o) == "line1\nline2\tx\r");
	// only template text is translated, never inserted values
	CHECK(Render("{{v}}", &o) == "keep\\n");
}

TEST_CASE("Dotted and indexed names resolve into nested objects and arrays") {
	JSONObject o;
	JSONObject *item = new JSONObject();
	item->set("name", std::string("Spirit"));
	JSONArray *stats = new JSONArray();
	stats->add(StatEntry(2, "to All Skills"));
	stats->add(StatEntry(35, "Faster Cast Rate"));
	item->set("stats", stats);
	o.set("item", item);
	CHECK(Render("{{item.name}}: {{item.stats.1.value}}% {{item.stats[1].name}}", &o) ==
		"Spirit: 35% Faster Cast Rate");
	CHECK(Render("[{{item.stats.2.name}}][{{item.nope.deeper}}]", &o) == "[][]");
}

TEST_CASE("Sections render once for truthy scalars and not at all for falsy or missing values") {
	JSONObject o;
	o.set("yes", true);
	o.set("no", false);
	o.set("zero", 0);
	o.set("num", 3);
	o.set("empty", std::string(""));
	o.set("str", std::string("s"));
	o.set("emptyList", new JSONArray());
	o.set("emptyObj", new JSONObject());
	CHECK(Render("{{#yes}}Y{{/yes}}", &o) == "Y");
	CHECK(Render("{{#num}}N{{/num}}", &o) == "N");
	CHECK(Render("{{#str}}S{{/str}}", &o) == "S");
	CHECK(Render("{{#no}}x{{/no}}{{#zero}}x{{/zero}}{{#empty}}x{{/empty}}", &o) == "");
	CHECK(Render("{{#missing}}x{{/missing}}{{#emptyList}}x{{/emptyList}}{{#emptyObj}}x{{/emptyObj}}", &o) == "");
	CHECK(Render("a{{#no}}b{{/no}}c", &o) == "ac");
}

TEST_CASE("An object section pushes the object as context and falls back to outer keys") {
	JSONObject o;
	o.set("name", std::string("Magic Find"));
	JSONObject *range = new JSONObject();
	range->set("min", 25);
	range->set("max", 50);
	o.set("range", range);
	CHECK(Render("{{#range}}{{name}} {{min}}-{{max}}{{/range}}", &o) == "Magic Find 25-50");
	// after the section, the outer context is active again
	CHECK(Render("{{#range}}{{min}}{{/range}}[{{min}}]", &o) == "25[]");
}

TEST_CASE("A list section renders its body once per element, in order") {
	JSONObject o;
	JSONArray *stats = new JSONArray();
	stats->add(StatEntry(2, "a"));
	stats->add(StatEntry(3, "b"));
	stats->add(StatEntry(4, "c"));
	o.set("stats", stats);
	o.set("sep", std::string(";"));
	CHECK(Render("{{#stats}}<{{value}} {{name}}>{{sep}}{{/stats}}", &o) == "<2 a>;<3 b>;<4 c>;");

	JSONArray *nums = new JSONArray();
	nums->add(1);
	nums->add(2);
	nums->add(3);
	o.set("nums", nums);
	CHECK(Render("{{#nums}}{{this}},{{/nums}}", &o) == "1,2,3,");
}

TEST_CASE("{{#this}} iterates a root-level array") {
	JSONArray root;
	root.add(Obj("type", "Shako"));
	root.add(Obj("type", "Jewel"));
	CHECK(Render("{{#this}}* {{type}}\n{{/this}}", &root) == "* Shako\n* Jewel\n");
}

TEST_CASE("Nested sections with the same name close at the matching tag") {
	JSONObject o;
	JSONObject *a = new JSONObject();
	a->set("a", std::string("inner"));
	o.set("a", a);
	CHECK(Render("{{#a}}[{{#a}}{{a}}{{/a}}]{{/a}}!", &o) == "[inner]!");
}

TEST_CASE("Inverted sections render only when the value is falsy, missing or an empty list") {
	JSONObject o;
	o.set("yes", true);
	o.set("no", false);
	o.set("zero", 0);
	o.set("empty", std::string(""));
	o.set("emptyList", new JSONArray());
	JSONArray *list = new JSONArray();
	list->add(1);
	o.set("list", list);
	o.set("name", std::string("ctx"));
	CHECK(Render("{{^no}}1{{/no}}{{^zero}}2{{/zero}}{{^empty}}3{{/empty}}{{^missing}}4{{/missing}}{{^emptyList}}5{{/emptyList}}", &o) == "12345");
	CHECK(Render("{{^yes}}x{{/yes}}{{^list}}x{{/list}}", &o) == "");
	// the body of an inverted section still sees the surrounding context
	CHECK(Render("{{^missing}}{{name}}{{/missing}}", &o) == "ctx");
}

TEST_CASE("A close tag that does not match the open section is a parse error") {
	CHECK(Mustache::parse("{{#a}}x{{/b}}") == nullptr);
	CHECK(Mustache::parse("{{#a}}{{#b}}x{{/a}}{{/b}}") == nullptr);
	JSONObject o;
	Mustache::Context ctx(&o, nullptr);
	CHECK(Mustache::renderTemplate(nullptr, ctx) == "");
}

TEST_CASE("Mustache::render renders a template string in one call") {
	JSONObject o;
	o.set("name", std::string("Shako"));
	Mustache::Context ctx(&o, nullptr);
	CHECK(Mustache::render("Found {{name}}{{#name}}!{{/name}}", ctx) == "Found Shako!");
}

TEST_CASE("{{#key=value}} renders only when the value's text equals exactly") {
	JSONObject o;
	o.set("name", std::string("Shako"));
	const std::string t = "{{#quality=Unique}}U:{{name}}/{{quality}}{{/quality}}";

	o.set("quality", std::string("Unique"));
	CHECK(Render(t, &o) == "U:Shako/Unique");
	o.set("quality", std::string("Magic"));
	CHECK(Render(t, &o) == "");
	o.set("quality", std::string("unique"));
	CHECK(Render(t, &o) == "");
	o.set("quality", std::string("Unique2"));
	CHECK(Render(t, &o) == "");

	o.set("sockets", 4);
	CHECK(Render("{{#sockets=4}}four{{/sockets}}{{#sockets=3}}three{{/sockets}}", &o) == "four");
}

TEST_CASE("{{#key!value}} renders only when the value's text differs") {
	JSONObject o;
	const std::string t = "{{#quality!Unique}}not unique{{/quality}}";
	o.set("quality", std::string("Rare"));
	CHECK(Render(t, &o) == "not unique");
	o.set("quality", std::string("Unique"));
	CHECK(Render(t, &o) == "");
}

TEST_CASE("{{#key>N}} and {{#key<N}} compare numerically and strictly") {
	JSONObject o;
	const std::string gt = "{{#iLevel>80}}high{{/iLevel}}";
	const std::string lt = "{{#iLevel<10}}low{{/iLevel}}";
	o.set("iLevel", 80);
	CHECK(Render(gt, &o) == "");
	o.set("iLevel", 81);
	CHECK(Render(gt, &o) == "high");
	o.set("iLevel", 10);
	CHECK(Render(lt, &o) == "");
	o.set("iLevel", 9);
	CHECK(Render(lt, &o) == "low");
	// numeric, not lexicographic: "9" > "10" as text but 9 < 10 as numbers
	o.set("iLevel", 9);
	CHECK(Render("{{#iLevel>10}}x{{/iLevel}}", &o) == "");
	o.set("iLevel", std::string("100"));
	CHECK(Render("{{#iLevel>99.5}}x{{/iLevel}}", &o) == "x");
}

TEST_CASE("{{#key$A|B}} renders when the value is one of the |-separated alternatives") {
	JSONObject o;
	const std::string t = "{{#quality$Magic|Rare}}MR{{/quality}}";
	o.set("quality", std::string("Magic"));
	CHECK(Render(t, &o) == "MR");
	o.set("quality", std::string("Rare"));
	CHECK(Render(t, &o) == "MR");
	o.set("quality", std::string("Unique"));
	CHECK(Render(t, &o) == "");
	o.set("quality", std::string("Mag"));
	CHECK(Render(t, &o) == "");
	o.set("quality", std::string("Magic|Rare"));
	CHECK(Render(t, &o) == "");
	o.set("quality", std::string("Set"));
	CHECK(Render("{{#quality$Set}}single{{/quality}}", &o) == "single");
}

TEST_CASE("{{#key^A|B}} renders when the value is none of the alternatives") {
	JSONObject o;
	const std::string t = "{{#quality^Unique|Magic|Rare}}other{{/quality}}";
	o.set("quality", std::string("Set"));
	CHECK(Render(t, &o) == "other");
	o.set("quality", std::string("Normal"));
	CHECK(Render(t, &o) == "other");
	o.set("quality", std::string("Unique"));
	CHECK(Render(t, &o) == "");
	o.set("quality", std::string("Magic"));
	CHECK(Render(t, &o) == "");
	o.set("quality", std::string("Rare"));
	CHECK(Render(t, &o) == "");
}

TEST_CASE("Comparison sections never render for a missing key") {
	JSONObject o;
	CHECK(Render("{{#quality=Unique}}a{{/quality}}{{#quality!Unique}}b{{/quality}}"
		"{{#quality>1}}c{{/quality}}{{#quality$Unique}}d{{/quality}}{{#quality^Unique}}e{{/quality}}", &o) == "");
}

TEST_CASE("Partials render with the current context and unknown partials render empty") {
	JSONObject o;
	o.set("name", std::string("Outer"));
	o.set("child", Obj("name", "Inner"));
	Partials p;
	p.Add("p", "<{{name}}>");
	CHECK(Render("{{>p}}{{#child}}{{>p}}{{/child}}{{>nope}}", &o, &p) == "<Outer><Inner>");
	// without a template factory, partial tags render nothing
	CHECK(Render("a{{>p}}b", &o) == "ab");
}

TEST_CASE("Partials nest and can recurse over nested data") {
	JSONObject o;
	JSONObject *a = new JSONObject();
	a->set("name", std::string("a"));
	JSONArray *kids = new JSONArray();
	kids->add(Obj("name", "b"));
	a->set("kids", kids);
	JSONArray *top = new JSONArray();
	top->add(a);
	o.set("kids", top);
	Partials p;
	p.Add("node", "({{name}}{{#kids}}{{>>node}}{{/kids}})");
	CHECK(Render("{{#kids}}{{>>node}}{{/kids}}", &o, &p) == "(a(b))");
}

TEST_CASE("Isolated partials {{>>name}} see only the current element, not outer contexts") {
	JSONObject o;
	o.set("name", std::string("Outer"));
	o.set("isEthereal", true);
	JSONObject *child = new JSONObject();
	child->set("type", std::string("Jewel"));
	o.set("child", child);
	Partials p;
	p.Add("p", "{{#isEthereal}}Eth {{/isEthereal}}{{type}}/{{name}}");
	p.Add("wrap", "[{{>p}}]");
	CHECK(Render("{{#child}}{{>p}}|{{>>p}}{{/child}}", &o, &p) == "Eth Jewel/Outer|Jewel/");
	// isolation drops parent data but keeps the template factory, so nested partials still resolve
	CHECK(Render("{{#child}}{{>>wrap}}{{/child}}", &o, &p) == "[Jewel/]");
}

TEST_CASE("Context::find searches the context chain from innermost to outermost") {
	JSONObject outer;
	outer.set("name", std::string("Outer"));
	outer.set("shared", std::string("outer-shared"));
	JSONObject inner;
	inner.set("shared", std::string("inner-shared"));
	inner.set("own", std::string("mine"));

	Mustache::Context root(&outer, nullptr);
	Mustache::Context child(&root, &inner);
	CHECK(child.find("own")->toString() == "mine");
	CHECK(child.find("shared")->toString() == "inner-shared");
	CHECK(child.find("name")->toString() == "Outer");
	CHECK(child.find("nowhere")->getType() == JSON_NULL);
	CHECK(child.find("this") == &inner);

	Mustache::Context isolated(child);
	CHECK(isolated.find("own")->toString() == "mine");
	CHECK(isolated.find("name")->getType() == JSON_NULL);
}

TEST_CASE("Context::findTemplate uses the nearest ancestor's template factory") {
	Partials p;
	p.Add("t", "T");
	JSONObject a, b;
	Mustache::Context root(&a, p.MakeFactory());
	Mustache::Context child(&root, &b);
	Mustache::Context grandchild(&child, &b);
	CHECK(grandchild.findTemplate("t") == p.templates["t"].get());
	CHECK(grandchild.findTemplate("unknown") == nullptr);
	Mustache::Context isolated(grandchild);
	CHECK(isolated.findTemplate("t") == p.templates["t"].get());

	Mustache::Context bare(&a, nullptr);
	Mustache::Context bareChild(&bare, &b);
	CHECK(bareChild.findTemplate("t") == nullptr);
}

// ---- Known bug ----

TEST_CASE("An integer 0 in the outermost context renders as 0" * doctest::should_fail()) {
	// BUG: JSONNumber::toString formats the integer 0 with "%f", so it renders "0.000000". A zero is
	// only printed when it is looked up in the outermost context (the template root, or the element
	// an isolated {{>>partial}} is rendered with): deeper lookups treat 0 as falsy and keep walking
	// up the parent chain. Integer JSON values must print as integers ("0", like every other int).
	JSONObject o;
	o.set("kills", 0);
	CHECK(Render("Kills: {{kills}}", &o) == "Kills: 0");

	JSONObject root;
	JSONObject *child = new JSONObject();
	child->set("sockets", 0);
	root.set("child", child);
	Partials p;
	p.Add("s", "[{{sockets}}]");
	// {{#child}} itself is entered because the object is non-empty; the isolated partial sees only it.
	CHECK(Render("{{#child}}{{>>s}}{{/child}}", &root, &p) == "[0]");
}

// ---- Default stash export templates (BHApp::stash.mustacheFormat defaults in BH.h) ----

namespace {
	// Mirrors StashExport: every mustacheFormat entry is parsed and registered under its name.
	class DefaultStashTemplates : public Partials {
	public:
		DefaultStashTemplates() {
			for (auto it = App.stash.mustacheFormat.defValues.begin(); it != App.stash.mustacheFormat.defValues.end(); it++) {
				Add(it->first, it->second);
			}
		}

		std::string RenderAs(const std::string &name, JSONElement *data) {
			Mustache::Context ctx(data, MakeFactory());
			return Mustache::renderTemplate(templates[name].get(), ctx);
		}
	};

	JSONObject *Item(const std::string &quality, const std::string &type, int iLevel) {
		JSONObject *item = new JSONObject();
		item->set("quality", quality);
		item->set("type", type);
		item->set("iLevel", iLevel);
		return item;
	}

	JSONArray *Stats(JSONObject *a, JSONObject *b = nullptr, JSONObject *c = nullptr) {
		JSONArray *s = new JSONArray();
		s->add(a);
		s->add(b);
		s->add(c);
		return s;
	}

	JSONObject *Shako() {
		JSONObject *it = Item("Unique", "Shako", 87);
		it->set("name", std::string("Harlequin Crest"));
		it->set("defense", 141);
		it->set("stats", Stats(StatEntry(2, "to All Skills"),
			RangedStat(50, "% Better Chance of Getting Magic Items", 25, 50),
			StatEntry(1, "Skill", "Teleport")));
		return it;
	}

	JSONObject *Spirit() {
		JSONObject *it = Item("Normal", "Monarch", 85);
		it->set("isRuneword", true);
		it->set("runeword", std::string("Spirit"));
		it->set("sockets", 4);
		it->set("defense", 148);
		it->set("stats", Stats(StatEntry(2, "to All Skills")));
		JSONArray *socketed = new JSONArray();
		socketed->add(Item("Normal", "Tal Rune", 1));
		it->set("socketed", socketed);
		return it;
	}

	JSONObject *EthRareWithJewel() {
		JSONObject *it = Item("Rare", "Grim Helm", 80);
		it->set("name", std::string("Doom Visor"));
		it->set("isEthereal", true);
		it->set("sockets", 1);
		it->set("stats", Stats(StatEntry(20, "Life")));
		JSONObject *jewel = Item("Magic", "Jewel", 70);
		jewel->set("name", std::string("Ruby Jewel of Fervor"));
		jewel->set("stats", Stats(StatEntry(15, "Increased Attack Speed")));
		JSONArray *socketed = new JSONArray();
		socketed->add(jewel);
		it->set("socketed", socketed);
		return it;
	}

	JSONObject *SkullStack() {
		JSONObject *it = Item("Normal", "Perfect Skull", 1);
		it->set("count", 3);
		return it;
	}

	JSONObject *SetHelm() {
		JSONObject *it = Item("Set", "Death Mask", 66);
		it->set("name", std::string("Tal Rasha's Horadric Crest"));
		return it;
	}
}

TEST_CASE("Every default stash template parses") {
	for (auto it = App.stash.mustacheFormat.defValues.begin(); it != App.stash.mustacheFormat.defValues.end(); it++) {
		INFO(it->first);
		std::unique_ptr<Mustache::AMustacheTemplate> t(Mustache::parse(it->second));
		CHECK(t != nullptr);
	}
}

TEST_CASE("Default item template: unique header is bold with level, then defense and stat lines") {
	DefaultStashTemplates t;
	std::unique_ptr<JSONObject> shako(Shako());
	CHECK(t.RenderAs("item", shako.get()) ==
		"**Harlequin Crest** (L87)"
		"\n\n    >141 defense"
		"\n\n    > 2 to All Skills"
		"\n\n    > 50 (25-50) % Better Chance of Getting Magic Items"
		"\n\n    > 1 Teleport"
		"\n");
}

TEST_CASE("Default item template: an unidentified unique (no name) shows its base type") {
	DefaultStashTemplates t;
	std::unique_ptr<JSONObject> it(Item("Unique", "Shako", 87));
	CHECK(t.RenderAs("item", it.get()) == "**Shako** (L87)\n");
}

TEST_CASE("Default item template: runewords show bold runeword name, base, sockets and hide socketed runes") {
	DefaultStashTemplates t;
	std::unique_ptr<JSONObject> spirit(Spirit());
	CHECK(t.RenderAs("item", spirit.get()) ==
		"**Spirit** Monarch (L85)[4]"
		"\n\n    >148 defense"
		"\n\n    > 2 to All Skills"
		"\n");
}

TEST_CASE("Default item template: ethereal rare with a socketed jewel lists the jewel as a nested item") {
	// The jewel is rendered via {{>>item}}, so it must not inherit "Eth" or "[1]" from its parent.
	DefaultStashTemplates t;
	std::unique_ptr<JSONObject> helm(EthRareWithJewel());
	CHECK(t.RenderAs("item", helm.get()) ==
		"Eth **Doom Visor** (L80)[1]"
		"\n\n    > 20 Life"
		"\n\n  * **Ruby Jewel of Fervor** (L70)"
		"\n\n    > 15 Increased Attack Speed"
		"\n"
		"\n");
}

TEST_CASE("Default item template: plain and set items are not bold; stacked items show their count") {
	DefaultStashTemplates t;
	std::unique_ptr<JSONObject> skulls(SkullStack());
	std::unique_ptr<JSONObject> set(SetHelm());
	CHECK(t.RenderAs("item", skulls.get()) == "Perfect Skull (L1) **x3**\n");
	CHECK(t.RenderAs("item", set.get()) == "Tal Rasha's Horadric Crest (L66)\n");
}

TEST_CASE("Default stash template renders every item as a bullet separated by blank lines") {
	DefaultStashTemplates t;
	JSONArray data;
	data.add(Shako());
	data.add(Spirit());
	data.add(EthRareWithJewel());
	data.add(SkullStack());
	data.add(SetHelm());
	CHECK(t.RenderAs("stash", &data) ==
		"* **Harlequin Crest** (L87)\n\n    >141 defense\n\n    > 2 to All Skills"
		"\n\n    > 50 (25-50) % Better Chance of Getting Magic Items\n\n    > 1 Teleport\n\n\n"
		"* **Spirit** Monarch (L85)[4]\n\n    >148 defense\n\n    > 2 to All Skills\n\n\n"
		"* Eth **Doom Visor** (L80)[1]\n\n    > 20 Life\n\n  * **Ruby Jewel of Fervor** (L70)"
		"\n\n    > 15 Increased Attack Speed\n\n\n\n"
		"* Perfect Skull (L1) **x3**\n\n\n"
		"* Tal Rasha's Horadric Crest (L66)\n\n\n");

	JSONArray empty;
	CHECK(t.RenderAs("stash", &empty) == "");
}

}
