#include "shvtreenode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/coreqt/log.h>
#include <shv/core/exception.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace node {

chainpack::RpcValue ShvTreeNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	shv::chainpack::RpcValue ret = call2(rq.method().toString(), rq.params(), rq.shvPath().toString());
	if(rq.requestId().toUInt() == 0)
		return cp::RpcValue(); // RPC calls with requestID == 0 does not expect response
	return ret;
}

chainpack::RpcValue ShvTreeNode::dir2(const chainpack::RpcValue &methods_params, const std::string &shv_path)
{
	cp::RpcValue::List ret;
	chainpack::RpcValueGenList params(methods_params);
	const std::string method = params.value(0).toString();
	unsigned attrs = params.value(1).toUInt();
	size_t cnt = methodCount2(shv_path);
	for (size_t ix = 0; ix < cnt; ++ix) {
		const chainpack::MetaMethod *mm = metaMethod2(ix, shv_path);
		if(method.empty()) {
			ret.push_back(mm->attributes(attrs));
		}
		else if(method == mm->name()) {
				ret.push_back(mm->attributes(attrs));
				break;
		}
	}
	return ret;
}

ShvNode::StringList ShvTreeNode::methodNames2(const std::string &shv_path)
{
	ShvNode::StringList ret;
	size_t cnt = methodCount2(shv_path);
	for (size_t ix = 0; ix < cnt; ++ix) {
		ret.push_back(metaMethod2(ix, shv_path)->name());
	}
	return ret;
}

chainpack::RpcValue ShvTreeNode::ls2(const chainpack::RpcValue &methods_params, const std::string &shv_path)
{
	chainpack::RpcValueGenList mpl(methods_params);
	const std::string child_name_pattern = mpl.value(0).toString();
	unsigned attrs = mpl.value(1).toUInt();
	cp::RpcValue::List ret;
	for(const std::string &child_name : childNames2(shv_path)) {
		if(child_name_pattern.empty() || child_name_pattern == child_name) {
			std::string path = shv_path.empty()? child_name: shv_path + '/' + child_name;
			try {
				cp::RpcValue::List attrs_result = lsAttributes2(attrs, path).toList();
				if(attrs_result.empty()) {
					ret.push_back(child_name);
				}
				else {
					attrs_result.insert(attrs_result.begin(), child_name);
					ret.push_back(attrs_result);
				}
			}
			catch (std::exception &) {
				ret.push_back(nullptr);
			}
		}
	}
	return ret;
}

bool ShvTreeNode::hasChildren2(const std::string &shv_path)
{
	return !childNames2(shv_path).empty();
}

chainpack::RpcValue ShvTreeNode::lsAttributes2(unsigned attributes, const std::string &shv_path)
{
	shvLogFuncFrame() << "attributes:" << attributes << "path:" << shv_path;
	cp::RpcValue::List ret;
	if(attributes & (int)cp::LsAttribute::HasChildren) {
		ret.push_back(hasChildren2(shv_path));
	}
	return ret;
}

chainpack::RpcValue ShvTreeNode::call2(const std::string &method, const chainpack::RpcValue &params, const std::string &shv_path)
{
	shvLogFuncFrame() << "method:" << method << "params:" << params.toCpon() << "shv path:" << shv_path;
	if(method == cp::Rpc::METH_LS) {
		return ls2(params, shv_path);
	}
	if(method == cp::Rpc::METH_DIR) {
		return dir2(params, shv_path);
	}
	SHV_EXCEPTION("Invalid method: " + method + " called for node: " + shvPath());
}

} // namespace node
} // namespace iotqt
} // namespace shv
