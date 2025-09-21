# Data Processing Platform

Сервис обрабатывает потоковую информацию о звонках, операторах и абонентах,
проходящих через платформу телефонии и колл-центров компании. Сервис загружает
различную потоковую информацию о звонках, реализует RESTful API для получения
статистики звонков и выгружает совокупную информацию о звонках в формате CDR (Call
Detail Record) в сервисы-потребители.

- **DataSources** — FastAPI мок-сервер потоков данных
- **CDRClient** — FastAPI мок-клиент для загрузки отчётов
- **Service** — Основной сервис обработки на [userver](https://github.com/userver-framework/userver) и PostgreSQL

## Структура репозитория

```
.
├── DataSources/    # FastAPI mockserver — эмулирует внешние data endpoints
│   └── data/       # JSON-файлы с mock данными
│   └── mockserver.py
├── CDRClient/      # FastAPI mockclient — принимает готовые CDR отчеты
│   └── mockclient.py
├── Service/        # Основной сервис на userver + Postgres
│   └── ...
└── README.md
```

---

## Быстрый старт

### 1. Клонирование репозитория

```bash
git clone https://github.com/NikitaSemenov/CallFlowProcessor.git
cd CallFlowProcessor
```

### 2. Запуск мок-сервисов (DataSources и CDRClient)

Оба FastAPI сервиса требуют Python >=3.10.  
Рекомендуется запускать в отдельных терминалах.

#### 2.1 Создание виртуального окружения и установка зависимостей

```bash
python -m venv venv
source venv/bin/activate

pip install fastapi uvicorn
```

#### 2.2 Запуск DataSources (моксервер потоков данных)

```bash
cd DataSources
uvicorn mockserver:app --port 8001
```

Доступные эндпойнты:

- `GET /calls` — список звонков (курсорный вывод)
- `GET /call_events` — события звонков (курсорный вывод)
- `GET /connections` — соединения (курсорный вывод)
- `GET /operators` — операторы (курсорный вывод)

Данные берутся из папки `data/` внутри `DataSources`.

#### 2.3 Запуск CDRClient (клиент для приёма отчетов)

В новом терминале:

```bash
cd CDRClient
source ../venv/bin/activate
uvicorn mockclient:app --port 8002
```

Доступный эндпойнт:

- `POST /records` — выгрузка собранных CDR-отчетов

---

### 3. Запуск основного сервиса (Service)

#### Требования

- [Docker](https://www.docker.com/)
- [docker-compose](https://docs.docker.com/compose/)

#### Запуск

```bash
cd Service
docker compose up --build
```

Сервис автоматически развернёт необходимые контейнеры (call-flow-processor, postgres).

---

## Схема взаимодействия сервисов

```
   +-------------+       +-------------+       +-----------+
   | DataSources | ----> |   Service   | ----> | CDRClient |
   +-------------+       +-------------+       +-----------+
   |  /calls     |       |data_fethcers|       | /records  |
   |             |       |cdr_uploaders|       |           |
```

- Service периодически обращается к DataSources за данными звонков.
- Service обрабатывает полученные данные, формирует CDR отчеты.
- Service отправляет отчеты методом POST в CDRClient на эндпойнт `/records`.
