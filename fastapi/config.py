from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    # Neo4j
    NEO4J_URI: str = "bolt://baldo-neo4j:7687"
    NEO4J_USER: str = "neo4j"
    NEO4J_PASSWORD: str

    # LLM - OpenAI
    OPENAI_API_KEY: str = ""
    OPENAI_MODEL: str = "gpt-4o"

    # LLM - Anthropic (primario)
    ANTHROPIC_API_KEY: str = ""
    ANTHROPIC_MODEL: str = "claude-sonnet-5"

    # LLM - DeepSeek (API compatibile OpenAI, base_url diverso). "deepseek-flash"
    # è l'ID canonico corrente di DeepSeek-V4.1-Flash su api.deepseek.com.
    DEEPSEEK_API_KEY: str = ""
    DEEPSEEK_MODEL: str = "deepseek-flash"
    DEEPSEEK_BASE_URL: str = "https://api.deepseek.com"

    # LLM - Ollama (servizio interno Docker o locale)
    OLLAMA_BASE_URL: str = "http://ollama:11434"
    OLLAMA_MODEL: str = "llama3.2"

    # Motore astronomico (API Node.js interna)
    ASTRONOMY_API_URL: str = "http://baldo-app:3002/api/PLANETARIUM"

    # CORS
    CORS_ORIGINS: list[str] = ["http://localhost", "http://127.0.0.1"]

    OPENWEATHER_API_KEY: str 
    
    # Se vuoi anche configurare l'URL del meteo (opzionale)
    WEATHER_API_URL: str = "https://api.openweathermap.org/data/2.5/weather"
    
    class Config:
        env_file = "../.env"   # Usa il .env della root di BALDOWEB
        extra = "ignore"

settings = Settings()
