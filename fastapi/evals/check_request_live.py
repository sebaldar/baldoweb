"""Opt-in live regression: run from fastapi with python -m evals.check_request_live."""
import asyncio,json
from evals.run import make_router,close_router
from services.narrative_review import verifica_testo_finale
async def main():
 router=make_router('anthropic')
 results=[]
 try:
  for name in ('il pianeta','Saturno'):
   story=f'La mamma apre la mappa. «Cerchiamo {name} verso est, basso sull’orizzonte». Lupetto spinge il foglio con il righello e lo recupera. Le nuvole coprono tutto. «Non possiamo vederlo», dice la mamma. Tornano a guardare la mappa.'
   result=await verifica_testo_finale(dict(racconto_finale=story,prompt_originale='Racconta di Lupetto e la mamma che scelgono un pianeta realmente osservabile da Roma alle 21:00. Indica dove guardare. Se le nuvole lo coprono, non fingere di vederlo.',eta_bambino=6,usa_astronomia=True,dati_astronomici={'status':'reale','corpi':[{'nome':'Saturno','direzione_cardinale':'est','altezza_deg':9}]},luogo='Roma',ora_storia='21:00:00'),router)
   results.append({'caso':name,'testo':result['racconto_finale'],'revisione':result['revisione_finale']})
  print(json.dumps(results,ensure_ascii=False,indent=2))
  assert results[0]['revisione']['esito']=='corretto'
  assert 'Saturno' in results[0]['testo']
  assert results[1]['revisione']['esito']=='ok'
 finally:
  await close_router(router)
if __name__ == '__main__':
 asyncio.run(main())
