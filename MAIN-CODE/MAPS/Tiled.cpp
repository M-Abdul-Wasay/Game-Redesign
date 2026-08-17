#include <SFML/Graphics.hpp>
#include <iostream>
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/Tileset.hpp>
#include <vector>
#include <map>

class TileMap : public sf::Drawable, public sf::Transformable
{
public:
    bool load(const std::string& tmxPath)
    {
        if (!m_map.load(tmxPath))
        {
            std::cerr << "Failed to load map: " << tmxPath << std::endl;
            return false;
        }

        const auto& tilesets = m_map.getTilesets();
        const auto& layers = m_map.getLayers();

        for (const auto& layer : layers)
        {
            if (layer->getType() != tmx::Layer::Type::Tile)
                continue;

            const auto& tileLayer = layer->getLayerAs<tmx::TileLayer>();
            buildLayer(tileLayer, tilesets);
        }

        return true;
    }

    // Returns one rectangle per non-empty tile in the named layer, in unscaled map pixel coords.
    // Use this for walls/collision — pass the exact layer name as shown in Tiled (e.g. "Boundaries").
    std::vector<sf::FloatRect> getCollisionRects(const std::string& layerName) const
    {
        std::vector<sf::FloatRect> rects;
        auto tileSize = m_map.getTileSize();
        auto mapSize = m_map.getTileCount();

        for (const auto& layer : m_map.getLayers())
        {
            if (layer->getType() != tmx::Layer::Type::Tile) continue;
            if (layer->getName() != layerName) continue;

            const auto& tileLayer = layer->getLayerAs<tmx::TileLayer>();
            const auto& tiles = tileLayer.getTiles();

            for (unsigned y = 0; y < mapSize.y; ++y)
            {
                for (unsigned x = 0; x < mapSize.x; ++x)
                {
                    unsigned idx = y * mapSize.x + x;
                    if (tiles[idx].ID != 0)
                    {
                        rects.emplace_back(
                            static_cast<float>(x * tileSize.x),
                            static_cast<float>(y * tileSize.y),
                            static_cast<float>(tileSize.x),
                            static_cast<float>(tileSize.y)
                        );
                    }
                }
            }
        }
        return rects;
    }

private:
    tmx::Map m_map;

    struct LayerBatch
    {
        std::map<int, sf::VertexArray> vertsByTileset; // keyed by tileset firstGID
    };

    std::vector<LayerBatch> m_layers;
    std::map<int, sf::Texture> m_texturesByTileset; // loaded once, shared across all layers

    void buildLayer(const tmx::TileLayer& layer, const std::vector<tmx::Tileset>& tilesets)
    {
        auto mapSize = m_map.getTileCount();
        auto tileSize = m_map.getTileSize();

        LayerBatch batch;
        const auto& tileIDs = layer.getTiles();

        for (unsigned y = 0; y < mapSize.y; ++y)
        {
            for (unsigned x = 0; x < mapSize.x; ++x)
            {
                unsigned idx = y * mapSize.x + x;
                unsigned gid = tileIDs[idx].ID; // tmxlite already strips flip bits out of ID
                if (gid == 0) continue; // empty tile

                const tmx::Tileset* tileset = nullptr;
                for (const auto& ts : tilesets)
                {
                    if (gid >= ts.getFirstGID() &&
                        (ts.getLastGID() == 0 || gid <= ts.getLastGID()))
                    {
                        tileset = &ts;
                    }
                }
                if (!tileset) continue;

                int tsKey = static_cast<int>(tileset->getFirstGID());

                // load texture once, shared globally across all layers
                if (m_texturesByTileset.find(tsKey) == m_texturesByTileset.end())
                {
                    sf::Texture tex;
                    tex.loadFromFile(tileset->getImagePath());
                    m_texturesByTileset[tsKey] = tex;
                }

                // create this layer's vertex array for this tileset if needed
                if (batch.vertsByTileset.find(tsKey) == batch.vertsByTileset.end())
                    batch.vertsByTileset[tsKey] = sf::VertexArray(sf::Quads);

                unsigned localID = gid - tileset->getFirstGID();
                unsigned tsCols = tileset->getColumnCount();
                float tu = (localID % tsCols) * tileset->getTileSize().x;
                float tv = (localID / tsCols) * tileset->getTileSize().y;
                float tw = static_cast<float>(tileset->getTileSize().x);
                float th = static_cast<float>(tileset->getTileSize().y);

                // --- read this tile's flip/rotation flags ---
                std::uint8_t flags = tileIDs[idx].flipFlags;
                bool flipD = flags & tmx::TileLayer::FlipFlag::Diagonal;
                bool flipH = flags & tmx::TileLayer::FlipFlag::Horizontal;
                bool flipV = flags & tmx::TileLayer::FlipFlag::Vertical;

                sf::Vector2f texTopLeft(tu, tv);
                sf::Vector2f texTopRight(tu + tw, tv);
                sf::Vector2f texBottomRight(tu + tw, tv + th);
                sf::Vector2f texBottomLeft(tu, tv + th);

                // apply flips/rotation in the order the TMX spec expects:
                // diagonal (transpose) first, then horizontal, then vertical
                if (flipD)
                {
                    std::swap(texTopRight, texBottomLeft);
                }
                if (flipH)
                {
                    std::swap(texTopLeft, texTopRight);
                    std::swap(texBottomLeft, texBottomRight);
                }
                if (flipV)
                {
                    std::swap(texTopLeft, texBottomLeft);
                    std::swap(texTopRight, texBottomRight);
                }

                sf::Vertex quad[4];
                quad[0].position = sf::Vector2f(x * tileSize.x, y * tileSize.y);
                quad[1].position = sf::Vector2f((x + 1) * tileSize.x, y * tileSize.y);
                quad[2].position = sf::Vector2f((x + 1) * tileSize.x, (y + 1) * tileSize.y);
                quad[3].position = sf::Vector2f(x * tileSize.x, (y + 1) * tileSize.y);

                quad[0].texCoords = texTopLeft;
                quad[1].texCoords = texTopRight;
                quad[2].texCoords = texBottomRight;
                quad[3].texCoords = texBottomLeft;

                for (int i = 0; i < 4; ++i)
                    batch.vertsByTileset[tsKey].append(quad[i]);
            }
        }

        m_layers.push_back(std::move(batch));
    }

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        states.transform *= getTransform();

        // draw layers in order, so floor/walls/decorations stack correctly
        for (const auto& layer : m_layers)
        {
            for (const auto& pair : layer.vertsByTileset)
            {
                int tsKey = pair.first;
                states.texture = &m_texturesByTileset.at(tsKey);
                target.draw(pair.second, states);
            }
        }
    }
};