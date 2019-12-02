//import shv.cpon;
import shv.rpcvalue;
import shv.rpcmessage;
import shv.log;

void main()
{
	RpcMessage m;
	m.setRequestId(RpcValue(1));
	logInfo(m.toCpon());
}