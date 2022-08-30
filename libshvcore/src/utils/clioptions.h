#pragma once

#include "../shvcoreglobal.h"

#include <shv/chainpack/rpcvalue.h>

#define CLIOPTION_QUOTE_ME(x) #x

#define CLIOPTION_GETTER_SETTER(ptype, getter_prefix, setter_prefix, name_rest) \
	CLIOPTION_GETTER_SETTER2(ptype, CLIOPTION_QUOTE_ME(getter_prefix##name_rest), getter_prefix, setter_prefix, name_rest)

#define CLIOPTION_GETTER_SETTER2(ptype, pkey, getter_prefix, setter_prefix, name_rest) \
	public: ptype getter_prefix##name_rest() const { \
		shv::chainpack::RpcValue val = value(pkey); \
		return rpcvalue_cast<ptype>(val); \
	} \
	protected: shv::core::utils::CLIOptions::Option& getter_prefix##name_rest##_optionRef() {return optionRef(pkey);} \
	public: bool getter_prefix##name_rest##_isset() const {return isValueSet(pkey);} \
	public: bool setter_prefix##name_rest(const ptype &val) {return setValue(pkey, val);}

namespace shv {
namespace chainpack { class RpcValue; }
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT CLIOptions
{
public:
	CLIOptions();
	virtual ~CLIOptions();

	using StringList = std::vector<std::string>;
public:
	class SHVCORE_DECL_EXPORT Option
	{
	private:
		struct Data
		{
			chainpack::RpcValue::Type type = chainpack::RpcValue::Type::Invalid;
			StringList names;
			chainpack::RpcValue value;
			chainpack::RpcValue defaultValue;
			std::string comment;
			bool mandatory = false;

			Data(chainpack::RpcValue::Type type = chainpack::RpcValue::Type::Invalid) : type(type), mandatory(false) {}
		};
		Data m_data;
	public:
		bool isValid() const {return m_data.type != chainpack::RpcValue::Type::Invalid;}

		Option& setNames(const StringList &names) {m_data.names = names; return *this;}
		Option& setNames(const std::string &name) {m_data.names = StringList{name}; return *this;}
		Option& setNames(const std::string &name1, const std::string &name2) {m_data.names = StringList{name1, name2}; return *this;}
		const StringList& names() const {return m_data.names;}
		Option& setType(chainpack::RpcValue::Type type) {m_data.type = type; return *this;}
		chainpack::RpcValue::Type type() const {return m_data.type;}
		Option& setValueString(const std::string &val_str);
		Option& setValue(const chainpack::RpcValue &val) {m_data.value = val; return *this;}
		chainpack::RpcValue value() const {return m_data.value;}
		Option& setDefaultValue(const chainpack::RpcValue &val) {m_data.defaultValue = val; return *this;}
		chainpack::RpcValue defaultValue() const {return m_data.defaultValue;}
		Option& setComment(const std::string &s) {m_data.comment = s; return *this;}
		const std::string& comment() const {return m_data.comment;}
		Option& setMandatory(bool b) {m_data.mandatory = b; return *this;}
		bool isMandatory() const {return m_data.mandatory;}
		bool isSet() const {return value().isValid();}
	public:
		Option() {}
	};
public:
	CLIOPTION_GETTER_SETTER2(bool, "abortOnException", is, set, AbortOnException)
	CLIOPTION_GETTER_SETTER2(bool, "help", is, set, Help)

	Option& addOption(const std::string &key, const Option &opt = Option());
	bool removeOption(const std::string &key);
	const Option& option(const std::string &name, bool throw_exc = true) const;
	Option& optionRef(const std::string &name);
	const std::map<std::string, Option>& options() const {return m_options;}

	void parse(int argc, char *argv[]);
	virtual void parse(const StringList &cmd_line_args);
	bool isParseError() const {return !m_parseErrors.empty();}
	bool isAppBreak() const {return m_isAppBreak;}
	StringList parseErrors() const {return m_parseErrors;}
	StringList unusedArguments() {return m_unusedArguments;}

	std::string applicationDir() const;
	std::string applicationName() const;
	void printHelp() const;
	void printHelp(std::ostream &os) const;
	void dump() const;
	void dump(std::ostream &os) const;

	bool optionExists(const std::string &name) const;
	chainpack::RpcValue::Map values() const;
	chainpack::RpcValue value(const std::string &name) const;
	chainpack::RpcValue value(const std::string &name, const chainpack::RpcValue default_value) const;
	/// value is explicitly set from command line or in config file
	/// defaultValue is not considered to be an explicitly set value
	bool isValueSet(const std::string &name) const;
	bool setValue(const std::string &name, const chainpack::RpcValue val, bool throw_exc = true);
protected:
	chainpack::RpcValue value_helper(const std::string &name, bool throw_exception) const;
	std::tuple<std::string, std::string> applicationDirAndName() const;
	std::string takeArg(bool &ok);
	std::string peekArg(bool &ok) const;
	void addParseError(const std::string &err);
private:
	std::map<std::string, Option> m_options;
	StringList m_arguments;
	size_t m_parsedArgIndex = 0;
	StringList m_unusedArguments;
	StringList m_parseErrors;
	bool m_isAppBreak = false;
	StringList m_allArgs;
};

class SHVCORE_DECL_EXPORT ConfigCLIOptions : public CLIOptions
{
private:
	typedef CLIOptions Super;
public:
	ConfigCLIOptions();
	~ConfigCLIOptions() override {}

	CLIOPTION_GETTER_SETTER(std::string, c, setC, onfig)
	CLIOPTION_GETTER_SETTER(std::string, c, setC, onfigDir)

	void parse(const StringList &cmd_line_args) override;
	bool loadConfigFile();

	void mergeConfig(const shv::chainpack::RpcValue &config_map);
	std::string configFile();
	std::string effectiveConfigDir();
protected:
	void mergeConfig_helper(const std::string &key_prefix, const shv::chainpack::RpcValue &config_map);
};

}}}

