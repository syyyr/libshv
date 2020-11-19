#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	//addOption("tester.enabled").setType(cp::RpcValue::Type::Bool).setNames("--tester")
	//		.setComment("Enable tester support, which will be accessible at tester node.")
	//		.setDefaultValue(false);
	setUser("poweruser");
	setPassword("weakpassword");
}
