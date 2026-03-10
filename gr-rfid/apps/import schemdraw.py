import schemdraw
import schemdraw.elements as elm

# 配置绘图风格
with schemdraw.Drawing() as d:
    d.config(unit=2.5) # 设置网格大小
    
    # --- 1. 射频输入 (Antenna & Source) ---
    ant = d.add(elm.Antenna().up().label('Antenna\n(RF Source)'))
    d.add(elm.Line().right().at(ant.start).length(1.5))
    node_in_top = d.here
    
    # 向下画地线参考
    d.add(elm.Line().down().at(ant.start).length(3))
    node_gnd_start = d.here
    d.add(elm.Ground())
    d.add(elm.Line().right().length(1.5))
    node_in_bot = d.here
    
    # --- 2. 阻抗匹配网络 (L-Match) ---
    # 串联电感
    d.add(elm.Inductor().right().at(node_in_top).label('$L_{match}$'))
    node_match = d.here
    
    # 并联电容到地
    d.add(elm.Capacitor().down().at(node_match).label('$C_{match}$').to(node_in_bot.end[0] + 2.5, node_in_bot.end[1])) # 简单的对齐
    
    # 补全地线
    d.add(elm.Line().right().at(node_in_bot).to(node_match[0], node_in_bot.end[1]))
    node_gnd_match = d.here
    
    # --- 3. 反向散射调制 (Modulator) ---
    d.add(elm.Line().right().at(node_match).length(1.5))
    node_mod = d.here
    
    # 添加 NMOS 开关
    d.add(elm.Line().down().length(1))
    mos = d.add(elm.NFet(anchor='drain').right().label('Modulator'))
    d.add(elm.Ground().at(mos.source))
    
    # 调制信号输入
    d.add(elm.Line().left().at(mos.gate).length(0.5).label('Baseband\nData', loc='left'))
    
    # --- 4. 整流电路 (Rectifier / Voltage Doubler) ---
    # 继续向右延伸
    d.add(elm.Line().right().at(node_mod).length(1.5))
    node_rect_in = d.here
    
    # 隔直电容 C_pump
    d.add(elm.Capacitor().right().label('$C_{pump}$'))
    node_mid = d.here
    
    # 二极管 D1 (下行到地)
    d.add(elm.Diode().down().label('$D_1$ (Clamp)'))
    d.add(elm.Ground())
    
    # 二极管 D2 (继续向右整流)
    d.add(elm.Diode().right().at(node_mid).label('$D_2$ (Rectify)'))
    node_out = d.here
    
    # --- 5. 储能与负载 ---
    # 储能电容 C_store
    d.add(elm.Line().down().length(1))
    d.add(elm.Capacitor().down().label('$C_{store}$'))
    d.add(elm.Ground())
    
    # 负载电阻 R_load (并联)
    d.add(elm.Line().right().at(node_out).length(2))
    d.add(elm.Line().down().length(1))
    d.add(elm.Resistor().down().label('$R_{Load}$\n(Chip Logic)'))
    d.add(elm.Ground())
    
    # --- 标记输出 ---
    d.add(elm.Line().right().at(node_out).length(2.5))
    d.add(elm.Dot(open=True).label('$V_{DD}$ (DC Out)', loc='right'))

    # 添加虚线框标注 (使用 Python 代码手动绘制矩形框比较麻烦，通常建议后期添加，或者用复杂的 add_element)
    # 这里仅展示核心连接
    
    d.draw()
    # d.save('rfid_frontend.svg') # 如果需要保存