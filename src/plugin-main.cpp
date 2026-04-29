#include <obs-module.h>
#include <obs-websocket-api.h>
#include <util/base.h>
#include <util/platform.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-output-screenshot", "en-US")

#define VENDOR_NAME    "OutputScreenShot"
#define REQUEST_MSG    "getOutputScreenShot"
#define PLUGIN_LOG_TAG "[OutputScreenShot] "

// ── Base64 encoder ────────────────────────────────────────────────────────────

static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const uint8_t *data, size_t len)
{
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t b = (uint32_t)data[i] << 16;
        if (i + 1 < len) b |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < len) b |= (uint32_t)data[i + 2];
        out += B64[(b >> 18) & 0x3F];
        out += B64[(b >> 12) & 0x3F];
        out += (i + 1 < len) ? B64[(b >> 6) & 0x3F] : '=';
        out += (i + 2 < len) ? B64[(b >> 0) & 0x3F] : '=';
    }
    return out;
}

// ── Minimal PNG writer ────────────────────────────────────────────────────────
// Writes a valid PNG from raw RGBA data without any external dependency.

static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, size_t len)
{
    static uint32_t table[256];
    static bool table_ready = false;
    if (!table_ready) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        table_ready = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < len; i++)
        crc = table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

static uint32_t adler32(const uint8_t *buf, size_t len)
{
    uint32_t s1 = 1, s2 = 0;
    for (size_t i = 0; i < len; i++) {
        s1 = (s1 + buf[i]) % 65521;
        s2 = (s2 + s1)     % 65521;
    }
    return (s2 << 16) | s1;
}

static void write_u32_be(std::vector<uint8_t> &v, uint32_t x)
{
    v.push_back((x >> 24) & 0xFF);
    v.push_back((x >> 16) & 0xFF);
    v.push_back((x >>  8) & 0xFF);
    v.push_back((x >>  0) & 0xFF);
}

static void write_chunk(std::vector<uint8_t> &png,
                        const char type[4],
                        const uint8_t *data, size_t len)
{
    write_u32_be(png, (uint32_t)len);
    size_t crc_start = png.size();
    png.insert(png.end(), type, type + 4);
    if (len) png.insert(png.end(), data, data + len);
    uint32_t crc = crc32_update(0, png.data() + crc_start, 4 + len);
    write_u32_be(png, crc);
}

// Deflate store (no compression) — maximally simple, browsers handle it fine.
static std::vector<uint8_t> deflate_store(const uint8_t *data, size_t len)
{
    std::vector<uint8_t> out;
    // zlib header: CM=8, CINFO=7, FCHECK such that header % 31 == 0
    out.push_back(0x78);
    out.push_back(0x01);

    size_t offset = 0;
    while (offset < len) {
        size_t block = std::min<size_t>(len - offset, 65535);
        bool last    = (offset + block >= len);
        out.push_back(last ? 0x01 : 0x00); // BFINAL, BTYPE=00 (no compression)
        uint16_t blen = (uint16_t)block;
        out.push_back( blen        & 0xFF);
        out.push_back((blen >>  8) & 0xFF);
        out.push_back(~blen        & 0xFF);
        out.push_back((~blen >> 8) & 0xFF);
        out.insert(out.end(), data + offset, data + offset + block);
        offset += block;
    }

    uint32_t adl = adler32(data, len);
    write_u32_be(out, adl);
    return out;
}

static std::vector<uint8_t> rgba_to_png(const uint8_t *rgba,
                                         uint32_t w, uint32_t h)
{
    // Build filtered scanlines (filter byte 0 = None for each row)
    std::vector<uint8_t> raw;
    raw.reserve((size_t)(w * 4 + 1) * h);
    for (uint32_t y = 0; y < h; y++) {
        raw.push_back(0); // filter type None
        raw.insert(raw.end(),
                   rgba + (size_t)y * w * 4,
                   rgba + (size_t)y * w * 4 + w * 4);
    }

    auto idat_data = deflate_store(raw.data(), raw.size());

    std::vector<uint8_t> png;
    // PNG signature
    const uint8_t sig[] = {137,80,78,71,13,10,26,10};
    png.insert(png.end(), sig, sig + 8);

    // IHDR
    uint8_t ihdr[13];
    ihdr[0]  = (w >> 24) & 0xFF; ihdr[1]  = (w >> 16) & 0xFF;
    ihdr[2]  = (w >>  8) & 0xFF; ihdr[3]  = (w >>  0) & 0xFF;
    ihdr[4]  = (h >> 24) & 0xFF; ihdr[5]  = (h >> 16) & 0xFF;
    ihdr[6]  = (h >>  8) & 0xFF; ihdr[7]  = (h >>  0) & 0xFF;
    ihdr[8]  = 8;  // bit depth
    ihdr[9]  = 6;  // color type RGBA
    ihdr[10] = 0;  // compression
    ihdr[11] = 0;  // filter
    ihdr[12] = 0;  // interlace
    write_chunk(png, "IHDR", ihdr, 13);

    // IDAT
    write_chunk(png, "IDAT", idat_data.data(), idat_data.size());

    // IEND
    write_chunk(png, "IEND", nullptr, 0);

    return png;
}

// ── Screenshot capture ────────────────────────────────────────────────────────

static std::string capture_program_output_as_base64_png()
{
    // Get the main output video info
    struct obs_video_info ovi = {};
    if (!obs_get_video_info(&ovi)) {
        blog(LOG_WARNING, PLUGIN_LOG_TAG "obs_get_video_info failed");
        return "";
    }

    uint32_t w = ovi.output_width;
    uint32_t h = ovi.output_height;

    // Allocate staging texture and read pixels via obs_enter_graphics()
    std::vector<uint8_t> rgba(w * h * 4, 0);

    obs_enter_graphics();

    gs_texrender_t *texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
    gs_stagesurf_t *stagesurface = gs_stagesurface_create(w, h, GS_RGBA);

    bool captured = false;

    if (gs_texrender_begin(texrender, w, h)) {
        struct vec4 clear_color;
        vec4_zero(&clear_color);
        gs_clear(GS_CLEAR_COLOR, &clear_color, 1.0f, 0);
        gs_ortho(0.0f, (float)w, 0.0f, (float)h, -100.0f, 100.0f);

        obs_render_main_texture();

        gs_texrender_end(texrender);

        gs_stage_texture(stagesurface,
                         gs_texrender_get_texture(texrender));

        uint8_t *data = nullptr;
        uint32_t linesize = 0;
        if (gs_stagesurface_map(stagesurface, &data, &linesize)) {
            if (linesize == w * 4) {
                memcpy(rgba.data(), data, (size_t)w * h * 4);
            } else {
                // Handle non-packed rows
                for (uint32_t y = 0; y < h; y++)
                    memcpy(rgba.data() + (size_t)y * w * 4,
                           data + (size_t)y * linesize,
                           w * 4);
            }
            gs_stagesurface_unmap(stagesurface);
            captured = true;
        }
    }

    gs_stagesurface_destroy(stagesurface);
    gs_texrender_destroy(texrender);

    obs_leave_graphics();

    if (!captured) {
        blog(LOG_WARNING, PLUGIN_LOG_TAG "Failed to capture program output");
        return "";
    }

    auto png = rgba_to_png(rgba.data(), w, h);
    return base64_encode(png.data(), png.size());
}

// ── obs-websocket vendor glue ─────────────────────────────────────────────────

static obs_websocket_vendor vendor = nullptr;

static void on_request(obs_data_t *request_data,
                        obs_data_t *response_data,
                        void * /*priv*/)
{
    // request_data contains { "message": "getOutputScreenShot" }
    const char *msg = obs_data_get_string(request_data, "message");
    if (!msg || strcmp(msg, REQUEST_MSG) != 0)
        return;

    std::string b64 = capture_program_output_as_base64_png();
    if (b64.empty()) {
        obs_data_set_bool(response_data, "success", false);
        return;
    }

    // Fire a VendorEvent back to all websocket clients so the browser picks
    // it up via obs.on('VendorEvent', ...) — same pattern as AdvancedSceneSwitcher
    obs_data_t *event_data = obs_data_create();
    obs_data_set_string(event_data, "message", b64.c_str());
    obs_websocket_vendor_emit_event(vendor, "AdvancedSceneSwitcherMessage", event_data);
    obs_data_release(event_data);

    obs_data_set_bool(response_data, "success", true);
}

// ── OBS module lifecycle ──────────────────────────────────────────────────────

bool obs_module_load(void)
{
    blog(LOG_INFO, PLUGIN_LOG_TAG "Loading plugin v%s", PLUGIN_VERSION);
    return true;
}

// obs_module_post_load is required by obs-websocket-api — vendor registration
// must happen after all modules (including obs-websocket) have loaded.
void obs_module_post_load(void)
{
    vendor = obs_websocket_register_vendor(VENDOR_NAME);
    if (!vendor) {
        blog(LOG_WARNING, PLUGIN_LOG_TAG
             "obs_websocket_register_vendor failed — obs-websocket not loaded?");
        return;
    }

    bool ok = obs_websocket_vendor_register_request(
        vendor, "AdvancedSceneSwitcherMessage", on_request, nullptr);

    if (ok)
        blog(LOG_INFO, PLUGIN_LOG_TAG "Registered vendor request");
    else
        blog(LOG_ERROR, PLUGIN_LOG_TAG "Failed to register vendor request");
}

void obs_module_unload(void)
{
    blog(LOG_INFO, PLUGIN_LOG_TAG "Unloading plugin");
}
