#include <rest_rpc/client.hpp>

namespace client
{
	TIMAX_DEFINE_PROTOCOL(add, int(int, int));
	TIMAX_DEFINE_PROTOCOL(madoka, void(int, int));
}

using sync_client = timax::rpc::sync_client<timax::rpc::msgpack_codec>;

int main(void)
{
	timax::log::get().init("rest_rpc_client.lg");

	boost::asio::io_service io;
	sync_client client{ io };
	client.connect("127.0.0.1", "9000");

	try
	{
		int lhs = 1, rhs = 2;

		while (true)
		{
			using namespace std;

			client.pub(client::add, lhs, rhs);
			++rhs;

			std::this_thread::sleep_for(1s);
		}
	}
	catch (timax::rpc::client_exception const& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}