#pragma once

namespace DataFeedHub {

	class ITradingInterface {
	public:
		virtual ~ITradingInterface() = default;

		virtual bool connect(const ConnectionParams& params) = 0;
		virtual void disconnect() = 0;

		virtual std::vector<std::shared_ptr<ITradingAccount>> getAccounts() = 0;

		virtual OrderResult executeOrder(const Order& order, const std::string& accountId) = 0;

		// Другие методы...
	};

}; // namespace DataFeedHub