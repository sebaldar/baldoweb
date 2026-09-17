"""
config.py
Settings centralizzati e factory LLM.
Provider supportati: ionos (default), anthropic, openai, gemini.
"""

from functools import lru_cache
from pydantic_settings import BaseSettings
from langchain_openai import ChatOpenAI
from langchain_anthropic import ChatAnthropic
from langchain_google_genai import ChatGoogleGenerativeAI

class Settings(BaseSettings):
    # IONOS
    ionos_api_key:    str = ""
    ionos_base_url:   str = "https://openai.inference.de-txl.ionos.com/v1"
    ionos_model:      str = "openai/gpt-oss-120b"

    # ANTHROPIC
    anthropic_api_key: str = ""
    anthropic_model:   str = "claude-sonnet-4-20250514"

    # OPENAI
    openai_api_key: str = ""
    openai_model:   str = "gpt-4o-mini"

	# GEMINI
    google_api_key: str = ""
    gemini_model:   str = "gemini-2.5-flash"  # <--- Usiamo il modello veloce di nuova generazione!

    default_provider: str = "ionos"

    qdrant_host: str = "qdrant"
    qdrant_port: int = 6333

    openmeteo_url: str = "https://api.open-meteo.com/v1/forecast"

    class Config:
        env_file = ".env"
        extra    = "ignore"

@lru_cache
def get_settings() -> Settings:
    return Settings()

def get_llm(provider: str | None = None, streaming: bool = True):
    """
    Ritorna il LLM per il provider richiesto.
    Default: IONOS. Fallback automatico su Anthropic se IONOS non configurato.
    """
    s = get_settings()
    p = (provider or s.default_provider).lower()

    if p == "ionos":
        if not s.ionos_api_key:
            # Fallback silenzioso su Anthropic
            p = "anthropic"
        else:
            return ChatOpenAI(
                model=s.ionos_model,
                openai_api_key=s.ionos_api_key,
                openai_api_base=s.ionos_base_url,
                streaming=streaming,
                temperature=0.3,
            )

    if p == "anthropic":
        return ChatAnthropic(
            model=s.anthropic_model,
            api_key=s.anthropic_api_key,
            streaming=streaming,
            temperature=0.3,
        )

    if p == "openai":
        return ChatOpenAI(
            model=s.openai_model,
            openai_api_key=s.openai_api_key,
            streaming=streaming,
            temperature=0.3,
        )

    if p == "gemini":
        return ChatGoogleGenerativeAI(
            model=s.gemini_model,
            google_api_key=s.google_api_key,
            streaming=streaming,
            temperature=0.3,
        )

    raise ValueError(f"Provider sconosciuto: {provider}")
