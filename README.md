# Fullerene Chess Engine ($C_{72}$)

Játék: https://sakk.itk.ppke.hu/

Egy 3D-s, nem-euklideszi sakkváltozat C++ alapú játékmotorja, amely egy 72 mezős csonkított ikozaéder ($C_{72}$ fullerén) topológiáján működik. A projekt célja a matematikai gráfelmélet és az objektumorientált játékfejlesztés ötvözése, egy determinisztikus játéktér és szabályrendszer megalkotásával.

## 🏗️ Szoftverarchitektúra

A kód bázisa a "Separation of Concerns" (felelősségi körök szétválasztása) elvét követve két jól elkülöníthető, fejlécfájl-alapú rétegre bomlik:

*   **`fullerene_engine.h` (Topológia és Hálózat):** Magába foglalja a `FullereneGrid` osztályt, amely felépíti a 12 ötszögből és 60 hatszögből álló 3-reguláris gráfot. A memóriában determinisztikusan, fix matematikai szabályok alapján generálja le az orientált lapszomszédsági (`edgeNeighbors`) hálózatot, valamint halmazelméleti metszetképzéssel a csúcsszomszédsági (`vertexNeighbors`) hálózatot.
*   **`Fullerene_Chess.h` (Üzleti Logika és Játékmotor):** Tartalmazza a sakk szabályrendszerét, a bábuk lépéskalkulációját végző `Piece` osztályt, valamint a `GameEngine` komponenst, amely a köröket, az eseménykezelést és a győzelmi feltételeket menedzseli.

## ♟️ Játékszabályok és Bábuk

A játéktéren a hagyományos sakktól eltérő, egyedi szabályok érvényesülnek a 3D-s térgörbület miatt:

*   **Bástya (Rook):** A lapszomszédok mentén (az éleken) halad egyenes vonalban, amíg akadályba nem ütközik.
*   **Futó (Bishop):** Átlósan, a csúcsszomszédokon keresztül közlekedik. Egyedi blokád-szabály vonatkozik rá: az algoritmikus "sorompó" miatt nem haladhat át két olyan mező között (közös élen), amelyet két ellenséges bábu foglal el.
*   **Zászló (Flag):** A védendő célpont, amelynek leütése azonnali vereséget jelent. Alapvetően mozgásképtelen, de képes a **sáncolásra**: felcserélheti a helyét és a belső tulajdonságait (`PieceType` és `flagState`) egy vele azonos csapatban lévő, saját lapszomszédos Bástyával vagy Futóval.
*   **Ötszög-szabály:** A hálózat szimmetriatörése miatt, amint egy bábu ötszögre lép (legyen az bástya vagy futó), a mozgása arra a körre azonnal befejeződik, és nem haladhat tovább[cite: 10].

## 🚀 Főbb funkciók és technikai részletek

*   **Valós idejű útvonal-validáció:** A motor minden beérkező lépéskérésnél valós időben veti össze a bábuk elméleti mozgásterét a tábla pillanatnyi `boardState` foglaltságával.
*   **Optimalizált memóriakezelés (Sáncolás):** A bábuk mozgását és transzmutációját a dinamikusan változtatható `PieceType` tagváltozó határozza meg. A Zászló sáncolása egy hatékony, $O(1)$ komplexitású memóriaművelettel (az azonosítók és állapotkapcsolók megcserélésével) megy végbe, elkerülve a játék közbeni dinamikus objektum-allokációt.
*   **Beépített tesztelési keretrendszer:** A projekt tartalmazza az End-to-End validációhoz szükséges teszteket:
    *   `UnitTest()`: A teljes mozgástér leképezése minden egyes celláról indulva.
    *   `InitialStateTest()`: A kezdőállás összes szabályos lépésének kigenerálása.
    *   `RandomGameSimulation()`: Automatikus, randomizált mérkőzés-szimulátor (max. 300 lépésig)[cite: 10].
