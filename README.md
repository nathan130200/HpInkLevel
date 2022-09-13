# Hp Ink Level
Um software para uso pessoal para eu realizar monitoramento de níveis de tinta da minha impressora.

# Requisitos
- VCPKG
- ImGui
- ImGui SDL2
- ImGui SDL2 Renderer
- Impressora HP Deskjet 1510 Series
- Nlohmann Json C++ Library (já incluso no projeto)

# Notas
- De tempos em tempos ele irá verificar o arquivo que o próprio sistema da HP gera com as informações de níveis de tinta.

- Em breve irá fazer rodar script que irá invocar o app de Toolbox da HP para obter esse json atualizado.