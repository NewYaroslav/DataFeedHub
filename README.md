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

## Философия проекта

**DataFeedHub** (далее DFH) разрабатывается как универсальное "ядро" для торговых систем, которое бы позволяло проводить исторические тестирования, оптимизацию, поиск закономерностей и реальную торговлю.
Для универсальности реализация DFH не привязывается к конкертной бирже или брокеру, но даёт возможность встроить систему под любую торговую площадку.

Упор в DFH делается на производительность, чтобы можно было одновременно обрабатывать тысячи торговых активов. Этим система DFH должна отличаться от другихз распространенных решений.

## Особенности реализации

Для ускорения работы DFH имеет некоторые особенности, которые позволяют системе быть более производительной.

### Работа с активами

Все активы задаются тремя параметрами вместо использования строк, хотя это мешает при желании использовать и строковые представления.

Параметры:

1. market_type - Тип рынка. Имеет размер `3 бита`. Это перечисление определяет принадлежность актива к определенному типа рынка. Например, это могут быть бессрочные фьючерсы, инверсные бессрочные фьючерсы, опционы, спотовый рынок и т.д. 
2. exchange_id - ID биржи, провайдера ликвидности или брокера. Имеет размер `10 бит`. 
3. symbol_id - ID символа. Имеет размер `16 бит`. Так как данный ID является отдельным от типа рынка (market_type) или брокера (exchange_id), то это позволяет легко получить данные одного и того же симвоал на разных типах рынка или брокеров, что полезно для арбитражных стратегий.

Все три параметра образуют *32 битный ключ*, однако **фактически используется только первые `29 бит`**, что важно учитывать для *64-битных ключей* с данными метки времени. 

Структура ключа `32 бит`:

```
|empty (3)|market_type (3 bit)|exchange_id (10 bit)|exchange_id (16 bit)|
```

Получить *32 битный ключ* можно при помощи функции `make_symbol_key32`. 

### Работа с метриками





## Соглашения о именовании

- Для имен функций и методов используется стиль именования "глагол-сущность".
- Имена файлов с функциями, имена функций и методов придерживаются стиля snake_case. 
- Имена типов, классов и структур, а также файлов их содержащие, придерживаются стиля CamelCase. 
 