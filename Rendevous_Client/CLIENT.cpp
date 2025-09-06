#include "client.h"
#include "../SolutionHelper.cpp"

void Client::run()
{
    initAssets();

    while (wnd->isOpen())
    {
        handleWindowEvents(*wnd);
        processInput();
        Update(timer.restart().asSeconds());
        Render();
    }
}
Client::Client() : cellShp{ { (float)hlp::tileSize.first, (float)hlp::tileSize.second } } { wnd = new sf::RenderWindow{}; }
Client::~Client()
{
}

bool Client::OnUserCreate()
{



    uint32_t* data = mapData;

    if (!loadMap(&data, 1800, "FrenchTickla"))
    {
        std::cout << "Was not able to load the map tiles data into the array properly" << std::endl;
        return false;
    }



    settings.antiAliasingLevel = 8;
    wnd->create(sf::VideoMode{ {SCRW,SCRH},32U }, "Client Window", sf::State::Windowed, settings);
    tilesetTex.setSmooth(false);

    // (Intro kitty scene kept, unchanged for brevity)

    if (!wnd->isOpen()) return false;
    wnd->setPosition(sf::Vector2i(20, 120));

    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    // Build anim frames
    playerAnimFrames[(uint32_t)AnimID::Idle] = AnimSheet{};
    playerAnimFrames[(uint32_t)AnimID::Run] = AnimSheet{};
    playerAnimFrames[(uint32_t)AnimID::Attack] = AnimSheet{};




    // Idle
    for (int i = 0; i < 8; ++i) {
        playerAnimFrames[0][i].clear();
        playerAnimFrames[0][i].reserve(16);
        for (int j = 0; j < 16; ++j)
            playerAnimFrames[0][i].emplace_back(sf::IntRect{ {j * playerWidth, i * playerHeight},{playerWidth,playerHeight} });
    }
    // Run
    for (int i = 0; i < 8; ++i) {
        playerAnimFrames[1][i].clear();
        playerAnimFrames[1][i].reserve(17);
        for (int j = 0; j < 17; ++j)
            playerAnimFrames[1][i].emplace_back(sf::IntRect{ {j * playerWidth, i * playerHeight},{playerWidth,playerHeight} });
    }

    // Attack
    for (int i = 0; i < 8; ++i) {
        playerAnimFrames[2][i].clear();
        playerAnimFrames[2][i].reserve(17);
        for (int j = 0; j < 17; ++j)
            playerAnimFrames[2][i].emplace_back(sf::IntRect{ {j * playerWidth, i * playerHeight},{playerWidth,playerHeight} });
    }

    // Connect
    while (!Connect("127.0.0.1", 60000)) {}
    //24.236.104.52
    //
    //phone: 10.143.2.210
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
    //192.168.0.5
    return true;
}
void Client::initAssets()
{
    // Build tileset/tilemap (kept like your code)
    const int numCols = currTSetCols, numRows = currTSetRows, numTiles = numCols * numRows;
    tileset.reserve(numTiles);
    for (int i = 0; i < numTiles; i++) {
        int numx = i % numCols;
        int numy = i / numCols;
        auto& t = tileset.emplace_back(tilesetTex);
        t.setPosition({ 0.f,0.f });
        t.setTextureRect({ {numx * (int)hlp::tileSize.first, numy * (int)hlp::tileSize.second}, {(int)hlp::tileSize.first,(int)hlp::tileSize.second} });
    }

    const int mapCols = 40, mapRows = 45, total = mapCols * mapRows;
    tilemap.reserve(total);
    for (int y = 0; y < mapRows; y++)
    {
        for (int x = 0; x < mapCols; x++)
        {
            int pitch = mapCols;
            int num = y * pitch + x;
            if (num >= 1800) break;
            auto& t = tilemap.emplace_back(tilesetTex);
            t.setPosition({ (float)x * (float)hlp::tileSize.first, (float)y * (float)hlp::tileSize.second });
            t.setTextureRect({ sf::Vector2i{tileset[mapData[num]].getTextureRect().position},
                               sf::Vector2i{tileset[mapData[num]].getTextureRect().size} });
        }
    }

    selected.setTextureRect({ {4 * (int)hlp::tileSize.first, 9 * ((int)hlp::tileSize.second * 2)},{(int)hlp::tileSize.first,(int)hlp::tileSize.second}});
}
bool Client::loadMap(uint32_t** data, int numElems, const std::string& filename)
{
    std::string path{ "assets/areas/" };
    std::string ext{ ".map" };

    std::string fullRelativePath{ "" };
    fullRelativePath.append(path);
    fullRelativePath.append(filename);
    fullRelativePath.append(ext);

    std::ifstream iFile;
    //iFile.exceptions(std::ios::failbit || std::ios::badbit);
    iFile.open(fullRelativePath.c_str());

    if (iFile.is_open())
    {
        std::string fileCheck, areaName, sheetCheck, sheetName, mappingCheck, elevationCheck;
        int mapCols, mapRows, sheetCols, sheetRows, totalElevations;
        iFile >> fileCheck >> areaName >> mapCols >> mapRows >> sheetCheck >> sheetName >> sheetCols >> sheetRows >> mappingCheck >> totalElevations >> elevationCheck;

        assert(fileCheck == "area" && sheetCheck == "sheet" && mappingCheck == "mapping" && elevationCheck == "elevation");
        assert(numElems == (mapCols * mapRows));
        if (sheetName == "StarterSet")
        {
            assert(sheetRows == 10 && sheetCols == 11);
            currTSetCols = sheetCols;
            currTSetRows = sheetRows;
        }

        for (int i = 0; i < totalElevations; i++)
        {
            int elevationNum;
            iFile >> elevationNum;
            for (int y = 0; y < mapRows; y++)
            {
                for (int x = 0; x < mapCols; x++)
                {

                    int mapIdx = y * mapCols + x;
                    if (mapIdx >= numElems)
                    {
                        std::cout << "Tried to read beyond end of mapdata array.  Ill let it slide this time" << std::endl;
                        break;

                    }
                    int tsetNum;
                    iFile >> tsetNum;
                    (*data)[mapIdx] = uint32_t(tsetNum);
                }
            }
        }

        std::cout << "Loaded map file: " << path << areaName << ext << " successfully" << std::endl;
        iFile.close();

    }
    else
    {
        std::cout << "Error with area file " << filename << std::endl;
        return false;
    }

    return true;
}

void Client::handleWindowEvents(sf::RenderWindow& wnd_)
{
    while (const std::optional ev = wnd_.pollEvent())
    {
        if (ev->is<sf::Event::Closed>()) wnd_.close();
        if (auto keyRel = ev->getIf<sf::Event::KeyReleased>()) {
            if (keyRel->code == sf::Keyboard::Key::M) inEditMode = !inEditMode;
        }
    }
    mpos = wnd_.mapPixelToCoords(sf::Mouse::getPosition(wnd_));
}
void Client::processInput()
{
    static bool keydown = false;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I)) { keydown = true; }
    if (keydown) { keydown = false; sendMessageMsg(TEXT::POOPSIE); }

}
bool Client::Update(float fElapsedTime)
{


    std::pair<int32_t, int32_t> cellpos = { (int32_t)((float)mpos.x / hlp::tileSize.first), (int32_t)((float)mpos.y / hlp::tileSize.second) };
    std::pair<int32_t, int32_t> offset = { (int32_t)mpos.x % (int32_t)hlp::tileSize.first,(int32_t)mpos.y % (int32_t)hlp::tileSize.second };

    // clamp cellShpX and Y so that it does not refer to an imaginary cell wrt world space
    cellShpX = cellpos.second;// (uint32_t)std::min(std::max((int)cellpos.first, 0), (int)hlp::worldSize.first - 1);
    cellShpY = cellpos.second; //(uint32_t)std::min(std::max((int)cellpos.second, 0), (int)hlp::worldSize.second -1);
    cellOffsetX = (uint32_t)std::min(std::max((int)offset.first, 0), (int)hlp::tileSize.first - 1);
    cellOffsetY = (uint32_t)std::min(std::max((int)offset.first, 0), (int)hlp::tileSize.second - 1);

       /* auto mapIso2 = [&](const sf::Vector2f& pos) {
        float xIso = ((pos.x / (float)TW) - (pos.y / (float)TH)) * 0.5f * TW;
        float yIso = ((pos.x / (float)TW) + (pos.y / (float)TH)) * 0.5f * (TH / ((TW == TH) ? 2.f : 1.f));
        return sf::Vector2f{ xIso, yIso };
        };*/

    // Network receive
    if (IsConnected())
    {
        while (!Incoming().empty())
        {
            auto pkg = Incoming().pop_front();
            auto& msg = pkg.msg;

            switch (msg.header.id)
            {
            case Msg::Client_Accepted:
            {
                std::cout << "Hell yeah mothafucka!  We connected!\n";
                cnet::message<Msg> out;
                out.header.id = Msg::Client_RegisterWithServer;
                // Fill minimal desc; server will override id
                descPlayer.vPos = { 3.0f, 3.0f };
                out << descPlayer;
                Send(out);
                break;
            }
            case Msg::Client_AssignID:
            {

                msg >> nPlayerID;

                if (mapObjects.count(nPlayerID)) { bWaitingForConnection = false; }
                break;
            }

            case Msg::Game_AddPlayer:
            {

                sPlayerDescription desc;
                msg >> desc;

                mapObjects.insert_or_assign(desc.nUniqueID, desc);
                drawObjects.insert_or_assign(desc.nUniqueID, PlayerDrawData{});
                if (desc.nUniqueID == nPlayerID) { bWaitingForConnection = false; }

                break;
            }


            case Msg::Game_RemovePlayer:
            {
                uint32_t nRemovalID = 0;
                msg >> nRemovalID;
                mapObjects.erase(nRemovalID);
                drawObjects.erase(nRemovalID);
                break;
            }
            case Msg::Server_PlayerDrawSnapshot:
            {
                uint32_t count = 0; msg >> count;
                if (count == 0u)
                {
                    break;
                }
                for (uint32_t i = 0; i < count; ++i)
                {
                    PlayerDrawData d{};
                    msg >> d;
                    drawObjects[d.id] = d;
                    if (d.id == nPlayerID) {

                        playerPos = { d.xpos, d.ypos };
                        // clamp indices before using them
                        auto ai = (static_cast<uint32_t>(d.animID) <= static_cast<uint32_t>(AnimID::Attack)) ? static_cast<uint32_t>(d.animID) : 0;
                        auto di = (static_cast<uint32_t>(d.dir) < 8u) ? static_cast<uint32_t>(d.dir) : 0u;

                        const auto& frames = playerAnimFrames[ai][di];
                        if (!frames.empty()) {
                            playerAnimID = (ai == (uint32_t)1) ? AnimID::Run : ((ai == (uint32_t)2) ? AnimID::Attack : AnimID::Idle);
                            playerDir = static_cast<Dir>(di);
                            playerAnimFrameIdx++;
                            playerAnimFrameIdx = d.frameIndex % static_cast<uint32_t>(frames.size());
                        }
                    }
                }
            }
            break;
            case Msg::Server_TileCollided:
            {
                TileCollide tc;
                msg >> tc;
                tileCollidePos.insert_or_assign(tc.id, sf::Vector2f{ (float)tc.xpos,(float)tc.ypos });
                tileCollided.insert_or_assign(tc.id, (tc.isColliding == 1u) ? true : false);
            }
            break;
            default:
            {}
            break;
            }
        }

        // Flush any queued chat messages
        if (!messages.empty())
        {
            for (auto& m : messages)
            {
                cnet::message<Msg> out;
                out.header.id = Msg::Client_SendText;
                out << m.nUniqueID << m.message;
                Send(out);
            }
            messages.clear();
        }

        // ===== INPUT ONLY (no local simulation) =====
        if (!inEditMode)
        {
            const bool ready = (nPlayerID != 0) && (mapObjects.count(nPlayerID) != 0);

            if (IsConnected() && ready)
            {
                PlayerIO myIO{};
                moving = false;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                {
                    myIO.r = true; moving = true;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                    myIO.l = true; moving = true;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                    myIO.u = true; moving = true;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                    myIO.d = true; moving = true;
                }

                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
                {
                    myIO.space = true;
                    isAttacking = true;
                    startingAttack = true;
                }



                cnet::message<Msg> ioMsg;
                ioMsg.header.id = Msg::Client_IO;
                ioMsg << myIO;
                this->myIO = myIO;
                Send(ioMsg);

              /*  PlayerCollisionTiles colTiles;
                colTiles.playerId = nPlayerID;
                colTiles.mapTileCount = 1800;
                colTiles.pitch = 40;
                colTiles.tileSize = hlp::tileSize.first;
                auto isoppos = hlp::ToScreen((float)mpos.x, (float)mpos.y);
                float tileSX = (((float)isoppos.first * (float)hlp::tileSize.first) - 10.f);
                float tileSY = (((float)isoppos.second * (float)hlp::tileSize.second) - 10.f);
                float tileEX = (tileSX + 299.f + 20.f);
                float tileEY = (tileSY + 240.f + 20.f);
                colTiles.startTileXIdx = (int32_t)std::max((int32_t)(tileSX / (float)hlp::tileSize.first), 0i32);
                colTiles.endTileXIdx = (int32_t)std::min((int32_t)(tileEX / (int32_t)hlp::tileSize.first), (int32_t)hlp::worldSize.first - 1i32);
                colTiles.startTileYIdx = (int32_t)std::max((int32_t)(tileSY / (float)hlp::tileSize.second), 0i32);
                colTiles.endTileYIdx = (int32_t)std::min((int32_t)(tileEY / (int32_t)hlp::tileSize.second), (int32_t)hlp::worldSize.second - 1i32);
                

                std::cout << "startX: " << colTiles.startTileXIdx << ", endX: " << colTiles.endTileXIdx << " .. startY: " << colTiles.startTileYIdx << ", endY: " << colTiles.endTileYIdx << "\n";


                cnet::message<Msg> collideTilesMsg;
                collideTilesMsg.header.id = Msg::Client_CollideTiles;
                collideTilesMsg << colTiles;
                Send(collideTilesMsg);*/

                if (moving)
                {

                    TileLayer tLayer = {};
                    tLayer.tileSize = (int)hlp::tileSize.first;
                    tLayer.width = 40;
                    tLayer.height = 45;
                    tLayer.data = mapData;

                    auto feet = sf::Vector2i{ (int)playerPos.x, (int)playerPos.y };
                    uint32_t* mapDD = mapData;

                }
            }

        }


    }
    return true;
}
void Client::Render()
{

    wnd->clear(sf::Color::Blue);

    // Camera centers on local player (isometric mapped)
    auto cam = sf::Vector2f(drawObjects[nPlayerID].xpos + playerWidth * 0.5f, drawObjects[nPlayerID].ypos + playerHeight * 0.5f);
    auto vw = wnd->getView();
    vw.setCenter(cam);
    wnd->setView(vw);

    // Draw tilemap as isometric sprites (unchanged)
    for (int i = 0; i < (int)tilemap.size(); i++)
    {
        sf::Vector2f p = hlp::ToScreenIso(tilemap[i].getPosition());
        sf::Sprite isoSprite{ tilesetTex };
        isoSprite.setPosition(p);
        isoSprite.setTextureRect({ {tilemap[i].getTextureRect().position}, {(int)hlp::tileSize.first, (int)hlp::tileSize.second * 2} });
        wnd->draw(isoSprite);
    }

    sf::Vector2i cell = { (int)mpos.x / (int)hlp::tileSize.first, (int)mpos.y / (int)hlp::tileSize.second };
    sf::Vector2i offset = { (int)mpos.x % (int)hlp::tileSize.first, (int)mpos.y % (int)hlp::tileSize.second };

    sf::Vector2f selectedWorld =
    {
          ((float)cell.y - hlp::worldOrigin.second) + ((float)cell.x - hlp::worldOrigin.first),
          ((float)cell.y - hlp::worldOrigin.second) - ((float)cell.x - hlp::worldOrigin.first)
    };

    float tw = (float)hlp::tileSize.first;
    float th = (float)hlp::tileSize.second;

    if (offset.x < (int)((tw / 2.f)) && offset.y < int((th / 2.f)))
    {
        // we are in F4 lines quadrant
        if (((offset.x * -th) - (tw * offset.y) + ((th * tw) / 2)) > 0)
        {
            // we are outside diamond, sub 1 from x
            selectedWorld.x -= 1.f;
        }

        
    }
    else if (offset.x > int((tw / 2.f)) && offset.y < int((th / 2.f)))
    {
        // we are in F1 lines quadrant
        if (((offset.x * th) - (tw * offset.y) - ((th * tw) / 2)) > 0)
        {
            // we are outside diamond, sub 1 from y
            selectedWorld.y -= 1.f;
        }
       
    }
    else if (offset.x < int((tw / 2.f)) && offset.y > int((th / 2.f)))
    {
       // we are in F3
        if (((offset.x * th) - (tw * offset.y) + ((th * tw) / 2)) < 0)
        {
            // we are outside diamond add 1 to y
            selectedWorld.y += 1.f;
        }      
    }
    else if (offset.x > int((tw / 2.f)) && offset.y > int((th / 2.f)))
    {
        // we are in F2
        if (((offset.x * -th) - (tw * offset.y) + (3.f/2.f)*th * tw) < 0)
        {
            // we are outside diamond, add 1 to x
            selectedWorld.x += 1.f;
        }        
    }
    


    //cellShp.setSize({ (float)hlp::tileSize.first, (float)hlp::tileSize.second });
    //sf::Vector2i cellSpacePos = sf::Vector2i{ (int)((mpos.x) / (float)hlp::tileSize.first), (int)(mpos.y / (float)hlp::tileSize.second) };
    //cellShp.setPosition({ (float)cell.x * (float)hlp::tileSize.first, (float)cell.y * (float)hlp::tileSize.second });
    //cellShp.setFillColor(sf::Color::Transparent);
    //cellShp.setOutlineColor(sf::Color::Red);
    //cellShp.setOutlineThickness(1);
    //wnd->draw(cellShp);
    if ((selectedWorld.x >= 0) && (selectedWorld.y >= 0) && (selectedWorld.x < hlp::worldSize.first) && (selectedWorld.y < hlp::worldSize.second))
    {
        selected.setPosition(hlp::ToScreenIso({ (float)selectedWorld.x * (float)hlp::tileSize.first, (float)selectedWorld.y * (float)hlp::tileSize.second }));
        wnd->draw(selected);
    }

    
  

    std::vector<std::pair<uint32_t, std::pair<uint32_t, PlayerDrawData>>> sortme;
    sortme.clear();
    sortme.reserve(drawObjects.size());

    for (auto& kv : drawObjects)
    {
        sortme.emplace_back(std::pair{ (int)getPlayerZHeight(kv.first), std::pair{kv.first, drawObjects[kv.first]} });
    }

    sort(sortme.begin(), sortme.end(), [&](const std::pair<uint32_t, std::pair<uint32_t, PlayerDrawData>>& a, const std::pair<uint32_t, std::pair<uint32_t, PlayerDrawData>>& b)->bool {
        return (a.first < b.first);
        });





    for (int i = 0; i < sortme.size(); i++)
    {
        if (sortme[i].second.second.id == 0Ui32) { continue; }
        uint32_t id = sortme[i].second.first;
        uint32_t zHeight = sortme[i].first;
        const auto& d = drawObjects[sortme[i].second.first];

        // Clamp bad values coming off the wire
        const uint32_t ai = animIndex(d.animID);
        const uint32_t di = dirIndex(d.dir);

        const auto& frames = playerAnimFrames[ai][di];
        if (frames.empty()) continue;
   
        sf::Vector2f pos{ d.xpos, d.ypos };
        if (d.id == nPlayerID) 
        {
            if (tileCollided[d.id])
            {
                sf::RectangleShape tmp({ (float)hlp::tileSize.first, (float)hlp::tileSize.second });
                tmp.setPosition(tileCollidePos[d.id]);
                tmp.setFillColor(sf::Color::Red);
               // wnd->draw(tmp);
            }
        
        }
        //    playerPos = { d.xpos,d.ypos };
        //    sf::Vector2i pSpot = { (int)playerPos.x, (int)playerPos.y };
        //    std::cout << "\n" << std::to_string(pSpot.x) << ", " << std::to_string(pSpot.y) << std::endl;
        //    // Clamp to avoid weird keys
        //    playerAnimID = (((AnimID)animIndex(d.animID) == (AnimID)(uint32_t)1) ? AnimID::Run : ((d.animID == (AnimID)(uint32_t)2) ? AnimID::Attack : AnimID::Idle));
        //    playerDir = static_cast<Dir>(dirIndex(d.dir));
        //    playerAnimFrameIdx = frames.empty() ? 0u : (d.frameIndex % static_cast<uint32_t>(playerAnimFrames[animIndex(playerAnimID)][dirIndex(playerDir)].size()));
        //    //std::cout << "Im at " << mapIso(playerPos).x << ", " << mapIso(playerPos).y << std::endl;
        //}

        const auto idx = frames.empty() ? 0u : (d.frameIndex % static_cast<uint32_t>(frames.size()));

        sf::Sprite spr{ ((ai == animIndex(AnimID::Idle)) ? playerTexArr[0] : ((ai == animIndex(AnimID::Attack)) ? playerTexArr[2] : playerTexArr[1])) };

        // If your world is “logical” coords, iso-map them; otherwise just use d.xpos/d.ypos

        spr.setPosition(pos);

        spr.setTextureRect(frames[idx]);

        wnd->draw(spr);


    }



    for (auto& p : drawObjects)
    {
        if (p.first == 0) continue;
        // Optional nameplate/info for self
        sf::Text txt{ font1 };
        txt.setCharacterSize(24);
        txt.setFillColor(sf::Color::White);
        txt.setOutlineColor(sf::Color::Black);
        txt.setOutlineThickness(1);
        sf::Vector2f camP = { (float)p.second.xpos,(float)p.second.ypos };
        txt.setPosition({ camP.x + 110.f, camP.y - 5.f });
        txt.setString(std::to_string(p.first)); // add if wanted
        wnd->draw(txt);
    }

    
    sf::View vwL8er = wnd->getDefaultView();
    sf::View prevView = wnd->getView();
    wnd->setView(vwL8er);

    displayText(*wnd, mpos, cell, selectedWorld, offset);

    wnd->setView(prevView);


    wnd->display();
}

void Client::displayText(sf::RenderWindow& wnd_,const sf::Vector2f& mouseStr_,const sf::Vector2i& cellStr_, const sf::Vector2f& selectedStr_, const sf::Vector2i offsetStr_)
{
    sf::Text mouseTxt{ font1 };
    sf::Text cellTxt{ font1 };
    sf::Text selectedTxt{ font1 };
    sf::Text offsetTxt{ font1 };


    mouseTxt.setFillColor(sf::Color::White);
    cellTxt.setFillColor(sf::Color::White);
    selectedTxt.setFillColor(sf::Color::White);
    offsetTxt.setFillColor(sf::Color::White);

    mouseTxt.setCharacterSize(22u);
    cellTxt.setCharacterSize(22u);
    selectedTxt.setCharacterSize(22u);
    offsetTxt.setCharacterSize(22u);

    mouseTxt.setOutlineColor(sf::Color::Black);
    cellTxt.setOutlineColor(sf::Color::Black);
    selectedTxt.setOutlineColor(sf::Color::Black);
    offsetTxt.setOutlineColor(sf::Color::Black);

    mouseTxt.setOutlineThickness(1u);
    cellTxt.setOutlineThickness(1u);
    selectedTxt.setOutlineThickness(1u);
    offsetTxt.setOutlineThickness(1u);

    mouseTxt.setPosition({20.f,20.f});
    cellTxt.setPosition({20.f ,50.f });
    selectedTxt.setPosition({ 20.f, 80.f });
    offsetTxt.setPosition({ 20.f, 110.f});

    std::string mouseStr{    "Mouse     : " };
    std::string cellStr{     "Cell      : " };
    std::string selectedStr{ "Selected  : " };
    std::string offsetStr{ "Offset  : " };

    mouseStr.append(std::to_string((int)mouseStr_.x));
    mouseStr.append(", ");
    mouseStr.append(std::to_string((int)mouseStr_.y));
   

    cellStr.append(std::to_string(cellStr_.x));
    cellStr.append(", ");
    cellStr.append(std::to_string(cellStr_.y));
    
    sf::Vector2i selStr{};
    selStr.x = (std::min(std::max((int)selectedStr_.x, 0), hlp::worldSize.first - 1));
    selStr.y = (std::min(std::max((int)selectedStr_.y, 0), hlp::worldSize.second - 1));
    if ((int)selectedStr_.x < 0)
        selStr.y = 0;
    if ((int)selectedStr_.y < 0)
        selStr.x = 0;
    selectedStr.append(std::to_string(selStr.x));
    selectedStr.append(", ");
    selectedStr.append(std::to_string(selStr.y));

    offsetStr.append(std::to_string((int)offsetStr_.x));
    offsetStr.append(", ");
    offsetStr.append(std::to_string((int)offsetStr_.y));
 
    mouseTxt.setString(mouseStr);
    cellTxt.setString(cellStr);
    selectedTxt.setString(selectedStr);
    offsetTxt.setString(offsetStr);

    wnd_.draw(mouseTxt);
    wnd_.draw(cellTxt);
    wnd_.draw(selectedTxt);
    wnd_.draw(offsetTxt);



}

sf::Texture& Client::getPlayerTex(AnimID id_) { return playerTexArr[(int)id_]; }
float Client::getPlayerZHeight(uint32_t playerID)
{
    sf::Vector2f ppos = { drawObjects[playerID].xpos,drawObjects[playerID].ypos };
    sf::Vector2f ipos = { (ppos.x - ppos.y) * ((float)hlp::tileSize.first / 2.f) ,  (ppos.x + ppos.y) * ((float)hlp::tileSize.first / 4.f) };

    return ipos.y + playerZHeightOffset;
}
uint32_t Client::animIndex(AnimID a) {
    const auto v = static_cast<uint32_t>(a);
    return (v <= static_cast<uint32_t>(AnimID::Attack)) ? static_cast<int>(v) : 0; // clamp
}
uint32_t Client::dirIndex(Dir d) {
    const auto v = static_cast<uint32_t>(d);
    return (v < 8u) ? static_cast<int>(v) : 0; // clamp
}

void Client::sendMessageMsg(const TEXT t)
{
    if (!IsConnected()) return;
    sMessageFromClient m{};
    m.nUniqueID = nPlayerID;
    m.message = t;
    messages.push_back(m);

    if (!Incoming().empty()) return;

    cnet::message<Msg> out;
    out.header.id = Msg::Client_SendText;
    out << messages.back().nUniqueID;
    out << messages.back().message;
    Send(out);
    messages.pop_back();
}



