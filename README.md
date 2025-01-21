# DataFeedHub

**DataFeedHub** is a comprehensive system for storing, processing, and testing financial data, including quotes, funding rates, and order books. It supports historical backtesting, inter-exchange arbitrage, and real-time data feed handling. The system utilizes efficient data compression and multi-threading for high-performance analysis.

## Features

- **Data Storage**: Compressed storage of quotes, funding rates, and order books in a database.
- **Historical Testing**: Run backtests for trading strategies, including inter-exchange arbitrage and funding.
- **Real-Time Data Feeds**: Handle real-time data feeds for various assets and providers.
- **Multi-Provider Support**: Store and analyze data from multiple liquidity providers and exchanges.
- **Multi-Threaded Environment**: Optimized for high-performance testing and data processing.

## Components

1. **Database**: Efficient storage of compressed historical data for quotes, funding, and order books.
2. **Backtesting Module**: Tools for running tests on historical data, including inter-exchange arbitrage and funding strategies.
3. **Real-Time Data Feed Module**: Manage and process real-time data streams from multiple providers.
4. **Provider and Symbol Management**: Support for multiple currency pairs, assets, and data sources.

## Соглашения о именовании

- Для имен функций и методов используется стиль именования "глагол-сущность".
- Имена файлов с функциями, имена функций и методов придерживаются стиля snake_case. 
- Имена типов, классов и структур, а также файлов их содержащие, придерживаются стиля CamelCase. 
 