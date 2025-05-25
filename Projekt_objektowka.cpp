#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION            120
#endif

#define SHADOWMAP_RESOLUTION 2048
#define MAX_FILEPATH_SIZE    2048

RenderTexture2D LoadShadowmapRenderTexture(int width, int height);
void UnloadShadowmapRenderTexture(RenderTexture2D target);
void GcodeAnalizer(std::string file_path, std::vector<Vector4>& points);
class main_model { //glowny model
public:
    Model model1;
    Vector3 position;

    main_model(const char* modelPath, const char* texturePath,Shader shader, Vector3 pos = { 0.0f, 0.0f, 0.0f })  {
        position = pos;
        model1 = LoadModel(modelPath);
		model1.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture(texturePath); //dodanie tekstury
        for (int i = 0; i < model1.materialCount; i++) { //low key trzeba ogarnąć o co z tych chodzi, 
            model1.materials[i].shader = shader;
        }
    }

    ~main_model() {
		UnloadTexture(model1.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture); //zwolnienie tekstury
        UnloadModel(model1);
    }

    virtual void Draw() {
        DrawModelEx(model1, position, { 0.0f, 1.0f, 0.0f }, 0.0f, { 0.1f, 0.1f, 0.1f }, WHITE);
    }
};

class modele : public main_model { //polimorfizm
public:
    BoundingBox box;
    modele(const char* modelPath, const char* texturePath, Shader shader, Vector3 pos = { 0.0f, 0.0f, 0.0f }) : main_model(modelPath, texturePath, shader, pos) {
        // Teraz model1 powinien być zainicjalizowany (zakładając, że konstruktor main_model to robi)
            box = GetMeshBoundingBox(model1.meshes[0]);
    }

    ~modele() {
        //Nic nie potrzebne, bo używamy z klasy bazowej
    }

    void Draw() override {
        DrawModelEx(model1, position, { 0.0f, 1.0f, 0.0f }, 0.0f, { 0.1f, 0.1f, 0.1f }, WHITE);
        DrawBoundingBox(GetTransformedBoundingBox(), BLUE); 
    }

    BoundingBox GetTransformedBoundingBox() const {
        // skalowanie boxa i zeby wyswietlal sie na modelu
        BoundingBox scaled_box = box;

        scaled_box.min = Vector3Scale(scaled_box.min, scale)+ position;
        scaled_box.max = Vector3Scale(scaled_box.max, scale)+ position;

        return scaled_box;
    }

private:
    float scale = 0.1f;
};

class TargetPoint {
public:
    std::vector<Vector4> Target_positions;  

    TargetPoint(std::vector<Vector4> pos) {
		Target_positions = pos;
        speed = 0.0f;
        kolizja = false;
    }

    ~TargetPoint() {}

    void MoveToPoint(modele* x, modele* y, modele* z, float dt) {
        //korecja polozenia o polozenie poczatkowe
        kolizja = CheckCollisionBoxes(x->GetTransformedBoundingBox(), z->GetTransformedBoundingBox());
        speed = 0.01f ;
        if (!start_pos) {
			if (x->position.x > x_start && abs((x->position.x - x_start)) > speed) {
				x->position.x -= speed;
            }
            else if(abs((x->position.x - x_start)) > speed){
                x->position.x += speed;
            }

			if (x->position.y > z->position.y && kolizja == 0 ) {
				x->position.y -= speed;
				y->position.y -= speed;
			}
			else if(kolizja ==0) {
				y->position.y += speed;
				x->position.y += speed;
			}

            if (z->position.z > z_start && abs((z->position.z - z_start)) > speed) {
               z->position.z -= speed;
            }
            else if(abs((z->position.z - z_start)) > speed){
               z->position.z += speed;
            }
            
            start_pos = kolizja && (abs(z->position.z - z_start) <= speed);

            if (start_pos == true) {
				y_start = x->position.y;
                offsetY = y->position.y - x->position.y;
            }
        }

        else {

            if (index >= Target_positions.size())
            {
                return;
            }

            //dodanie srodka do wektora + skalowanie
			if (srodek_dodany == false) { 
				for (int i = 0; i < Target_positions.size(); i++) { 
                					Target_positions[i].x = Target_positions[i].x / 10 + x_start;
                                    Target_positions[i].y = Target_positions[i].y / 10 + y_start;
                                    Target_positions[i].z = Target_positions[i].z / 10 + z_start;
									Target_positions[i].w = Target_positions[i].w / 10;
                }
                srodek_dodany = true;
            }

            Vector4 target = Target_positions[index]; 
            speed = target.w / 60 * dt; //prędkość ruchu na klatke animacji , wczytana z gcodea

            // Ruch w osi X
            if (x->position.x <= target.x && abs((x->position.x - target.x)) > speed) {
                x->position.x += speed;
            }
            else if (abs((x->position.x - target.x)) > speed) {
                x->position.x -= speed;
            }

            // Ruch w osi Y
            if (x->position.y <= target.y && abs((x->position.y - target.y)) > speed) {
                y->position.y += speed;
                x->position.y += speed;
            }
            else if (abs((x->position.y - target.y)) > speed) {
                y->position.y -= speed;
                x->position.y -= speed;
            }

            // Ruch w osi Z
            if (z->position.z <= target.z && abs((z->position.z - target.z)) > speed) {
                z->position.z += speed;
            }
            else if (abs((z->position.z - target.z)) > speed) {
                z->position.z -= speed;
            }

            //przejscie do nastepnego punktu
            if (abs((x->position.x - target.x)) <= speed && abs((x->position.y - target.y)) <= speed && abs((z->position.z - target.z)) <= speed) {
                x->position.x = target.x;
                y->position.y = target.y + offsetY;
                x->position.y = target.y;
                z->position.z = target.z;
                index++;
            }
        }
    }

	void clear() {
		//stan poczatkowy po wczytaniu nowego gcodea
		Target_positions.clear();
		index = 0;
		start_pos = false;
		srodek_dodany = false;
	}
private:
    bool kolizja = false;
    int index = 0;
    float offsetY = 0;
    bool start_pos = 0;
    bool srodek_dodany = 0;
    float speed ;
    float y_start = 0.0f;
    static constexpr float z_start = -11.7f;
    static constexpr float x_start = -12.3f;
};

int main(void) {

    const int screenWidth = 1600;
    const int screenHeight = 900;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Projekt obiektowka");

    Camera3D cam = { 0 };
    cam.position = { 0.0f, 60.0f, 100.0f };
    cam.target = { 0.0f, 25.0f, 0.0f };
    cam.projection = CAMERA_PERSPECTIVE;
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = 45.0f;

    Shader shadowShader = LoadShader(TextFormat("shadery/shader%i.vs", GLSL_VERSION), TextFormat("shadery/shader%i.fs", GLSL_VERSION));
    shadowShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shadowShader, "viewPos");
    Vector3 lightDir = Vector3Normalize({ 0.1f, -1.0f, -0.35f });
    Color lightColor = WHITE;
    Vector4 lightColorNormalized = ColorNormalize(lightColor);
    int lightDirLoc = GetShaderLocation(shadowShader, "lightDir");
    int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
    SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shadowShader, lightColLoc, &lightColorNormalized, SHADER_UNIFORM_VEC4);
    int ambientLoc = GetShaderLocation(shadowShader, "ambient");
    float ambient[4] = { 0.3f, 0.3f, 0.3f, 0.7f };
    SetShaderValue(shadowShader, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    int lightVPLoc = GetShaderLocation(shadowShader, "lightVP");
    int shadowMapLoc = GetShaderLocation(shadowShader, "shadowMap");
    int shadowMapResolution = SHADOWMAP_RESOLUTION;
    SetShaderValue(shadowShader, GetShaderLocation(shadowShader, "shadowMapResolution"), &shadowMapResolution, SHADER_UNIFORM_INT);

    //ładowanie modeli + klasy do ruchu
    TargetPoint cel({});
    main_model drukarka("modele/main.obj", "modele/main.png", shadowShader, {0.0f, 0.0f, 0.0f});
    modele table("modele/Y.obj", "modele/Y.png", shadowShader, { 0.0f, 5.55f, -6.3f });
    modele nozzle("modele/X.obj", "modele/X.png", shadowShader, { 0.0f, 31.33f, 0.0f });
    modele rail("modele/Z.obj", "modele/Z.png", shadowShader, { 0.0f, 30.0f, 0.0f });
    RenderTexture2D shadowMap = LoadShadowmapRenderTexture(SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
    Camera3D lightCam = { 0 };
    lightCam.position = Vector3Scale(lightDir, -15.0f);
    lightCam.target = Vector3Zero();
    lightCam.projection = CAMERA_ORTHOGRAPHIC;
    lightCam.up = { 0.0f, 1.0f, 0.0f };
    lightCam.fovy = 20.0f;

    DisableCursor();
    SetTargetFPS(200); //dalem wiecej klatek zeby ruch byl plynniejszy

    // Main game loop
    while (!WindowShouldClose()) {
		std::vector <Vector4> points;

        float dt = GetFrameTime();

        Vector3 cameraPos = cam.position;
        SetShaderValue(shadowShader, shadowShader.locs[SHADER_LOC_VECTOR_VIEW], &cameraPos, SHADER_UNIFORM_VEC3);
        UpdateCamera(&cam, CAMERA_FREE);

        const float cameraSpeed = 0.05f;
        if (IsKeyDown(KEY_LEFT)) {
            if (lightDir.x < 0.6f)
                lightDir.x += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            if (lightDir.x > -0.6f)
                lightDir.x -= cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_UP)) {
            if (lightDir.z < 0.6f)
                lightDir.z += cameraSpeed * 60.0f * dt;
        }
        if (IsKeyDown(KEY_DOWN)) {
            if (lightDir.z > -0.6f)
                lightDir.z -= cameraSpeed * 60.0f * dt;
        }
        lightDir = Vector3Normalize(lightDir);
        lightCam.position = Vector3Scale(lightDir, -15.0f);
        SetShaderValue(shadowShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);

        // Draw
        BeginDrawing();

        Matrix lightView;
        Matrix lightProj;
        BeginTextureMode(shadowMap);
        ClearBackground(WHITE);
        BeginMode3D(lightCam);
        lightView = rlGetMatrixModelview();
        lightProj = rlGetMatrixProjection();
        // RYSOWANIE

        DrawGrid(20, 10.0f); 

        EndMode3D();
        EndTextureMode();
        Matrix lightViewProj = MatrixMultiply(lightView, lightProj);

        ClearBackground(RAYWHITE);

        SetShaderValueMatrix(shadowShader, lightVPLoc, lightViewProj);

        rlEnableShader(shadowShader.id);
        int slot = 10; // Can be anything 0 to 15, but 0 will probably be taken up
        rlActiveTextureSlot(10);
        rlEnableTexture(shadowMap.depth.id);
        rlSetUniform(shadowMapLoc, &slot, SHADER_UNIFORM_INT, 1);

        BeginMode3D(cam);

        //wczytywanie z gcodea do vectora w klasie
      if (IsFileDropped()) {
            cel.clear();
            char filePath[MAX_FILEPATH_SIZE] = { 0 };
            FilePathList droppedFiles = LoadDroppedFiles();
            TextCopy(filePath, droppedFiles.paths[0]);
            std::cout << filePath << std::endl;
            GcodeAnalizer(std::string(filePath), cel.Target_positions);
            UnloadDroppedFiles(droppedFiles);
        }

        // Draw the same exact things as we drew in the shadowmap!
        // RYSOWANIE
        DrawGrid(20, 10.0f); //
        drukarka.Draw(); //
        table.Draw(); //
        nozzle.Draw(); //
        rail.Draw(); //

        //ruch do celu
        cel.MoveToPoint(&nozzle, &rail, &table, dt);

        EndMode3D();

        DrawFPS(10, 10);
        EndDrawing();

        //crazy , tez chyba mozna usunac
        if (IsKeyPressed(KEY_T)) {
            TakeScreenshot("shaders_shadowmap.png");
        }
    }

    // De-Initialization

    UnloadShader(shadowShader);
    UnloadShadowmapRenderTexture(shadowMap);

    CloseWindow();
    
    return 0;
}

RenderTexture2D LoadShadowmapRenderTexture(int width, int height) {
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer
    target.texture.width = width;
    target.texture.height = height;

    if (target.id > 0) {
        rlEnableFramebuffer(target.id);

        // Create depth texture
        // We don't need a color texture for the shadowmap
        target.depth.id = rlLoadTextureDepth(width, height, false);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach depth texture to FBO
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

// Unload shadowmap render texture from GPU memory (VRAM)
void UnloadShadowmapRenderTexture(RenderTexture2D target) {
    if (target.id > 0) {
        rlUnloadFramebuffer(target.id);
    }
}

void GcodeAnalizer(std::string file_path, std::vector<Vector4>& points) {  
  std::fstream gcode_file;  //otwieranie pliku
  gcode_file.open(file_path, std::ios::in);  
  if (gcode_file.is_open()) {  
      std::string line;
      float X = 0.0f; //nasze wartości pobierane z pliku, są tutaj, żeby nie były resetowane jeśli wartości się nie zmieniają pomiędzy linijkami
      float Y = 0.0f;
      float Z = 0.0f;
      float F = 0.0f;
	  while (std::getline(gcode_file, line)) { //dopuki nie znajdzie konca pliku
          if (line.rfind("G1", 0) == 0) {
			  int x_pos = line.find("X");  //szukamy pozycji X, Y, Z i F w linijce
              int y_pos = line.find("Y");  
              int z_pos = line.find("Z");
			  int f_pos = line.find("F");
              int space_pos = 0;  
			  if (x_pos > 0) {  //jeśli jest to znajdujemy spację i do tej pozycji pobieramy wartość
                  space_pos = line.find(" ", x_pos);  
                  X = std::stof(line.substr(x_pos + 1, space_pos - x_pos - 1));  
              }  
              space_pos = 0;  
              if (y_pos > 0) {  
                  space_pos = line.find(" ", y_pos);  
                  Y = std::stof(line.substr(y_pos + 1, space_pos - y_pos - 1));  
              }  
              space_pos = 0;  
              if (z_pos > 0) {  
                  space_pos = line.find(" ", z_pos);  
                  Z = std::stof(line.substr(z_pos + 1, space_pos - z_pos - 1));  
              }
              space_pos = 0;
              if (f_pos > 0) {
                  space_pos = line.find(" ", f_pos);
                  F = std::stof(line.substr(f_pos + 1, space_pos - f_pos - 1));
              }
			  points.push_back({ X, Z, Y, F });  //dodajemy do wektora punkty
          }  
      }  
      gcode_file.close();  
  }
  else {  //jeśli nie można otworzyć pliku
      std::cout << "Nie można otworzyć pliku gcode.txt" << std::endl;  
  }
  gcode_file.close(); //zamykamy plik jak dobre chłopaczki :)
}
