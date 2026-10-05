"""Scelte del form, singole o multiple, nel formato testuale dei report."""

GENERI_AMMESSI = {"amicizia", "avventura", "coraggio", "nanna", "magia"}
AMBIENTAZIONI_AMMESSE = {"bosco", "castello", "acqua", "montagna", "nuvole", "cielo", "giardino"}


def normalizza_scelte(value, ammesse):
    if not isinstance(value, str):
        return None
    scelte = dict.fromkeys(
        scelta.strip() for scelta in value.split(",") if scelta.strip() in ammesse
    )
    return ", ".join(scelte) or None
