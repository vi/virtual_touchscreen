#!/usr/bin/clojure
(ns org.vi-server.virtual-touchscreen
  "Virtual touchscreen pad"
  (:import
    (javax.swing 
     Action 
     JButton 
     JFrame 
     JLabel 
     JPanel 
     JTextField
     SwingUtilities
     JCheckBox
     )
    (java.net
     ServerSocket
    ))(:gen-class))

(def field-margins {:left 90, :top 30, :right 10, :bottom 10})
(def unused-area {:left 10, :top 10, :width 70, :height 250})
(def touchers-starting-position {:label-x 20, :label-y 40,   :check-x 40, :check-y 40})
(def touchers-sizes {:label-width 20, :label-height 20, :check-width 20, :check-height 20})
(def next-toucher-increment {:x 0, :y 20})
(def strings [
 {:text "Unused",     :x 15, :y 25}
 {:text "touchers",   :x 15, :y 35}
 {:text "Touch area (approximate)", :x 95, :y 45}
 {:text "Drag touchers into touch area to aim and check checkbox to touch.", :x 90, :y 15}
])
(def num-touchers 10)


(defn in-unused-area? [x y]
 (and 
  (< (:left unused-area) x (+ (:left unused-area) (:width unused-area)))
  (< (:top unused-area) y (+ (:top unused-area) (:height unused-area)))))


(defn create-painted-panel []
 (proxy [JPanel] []
  (paint [g] 
    (let [w (proxy-super getWidth)  h (proxy-super getHeight)]
    (proxy-super paint g)
    (.drawRect g 
     (:left unused-area)
     (:top unused-area)
     (:width unused-area)
     (:height unused-area)
    )
    (->> strings (map #(.drawString g (:text %) (:x %) (:y %))) (doall))

    (.drawRect g 
     (:left field-margins) 
     (:top field-margins) 
     (- w (+ (:left field-margins) (:right field-margins)))
     (- h (+ (:top field-margins) (:bottom field-margins))))
    )
   )
 ))


(defn create-touchscreen-window [events]
 (def tracking-id (atom 1))
 (def currently-pressed (atom 0))

 (defn send-string! [s]
   (locking events
    (swap! events (fn[_] s))
    (.notifyAll events)))

 (defn toucher-moved [i x y]
  ;; Send select-slot, touch-x, touch-y, x and y for mouse emulation   and sync messages
  (send-string! (format 
     "s %d\nX %d\nY %d\nx %d\ny %d\nS 0\n"
    i, x, y, x, y)))

 (defn toucher-active [i flag]
  ;; Send select-slot, tracking-id, touch-major-axis, pressure, trigger-mouse-emu and sync messages
  (send-string! (if flag
   (do (let [
        part1 (format "s %d\nT %d\n0 10\n: 100\ne 0\n", i, @tracking-id)
        _     (swap! tracking-id (fn[i] (if (> i 10000) 1 (inc i))))
        part2 (if (= @currently-pressed 0)
          "d 0\nS 0\n"
          "S 0\n")
        _     (swap! currently-pressed inc)
        ] (str part1 part2)))
   (do (let [
        part1 (format "s %d\nT -1\n0 0\n: 0\ne 0\n", i)
        part2 (if (= @currently-pressed 1)
          "u 0\nS 0\n"
          "S 0\n")
        _ (swap! currently-pressed dec)
       ] (str part1 part2))))))


 (let [
  panel (create-painted-panel)
  frame (JFrame.)
  checkboxes (map (fn[i] (let [c (JCheckBox.)   previous-value (atom false)]
    (.addChangeListener c (proxy [javax.swing.event.ChangeListener] []
     (stateChanged [e] 
        (let [new-value  (-> c (.getModel) (.isSelected))]
         (when (not= new-value @previous-value)
          (swap! previous-value not)
          (toucher-active i new-value))
      ))
     ))c))(range num-touchers))
  labels (map (fn[i] (let [l (JLabel. (str i))   c (nth checkboxes i)]
    (.addMouseMotionListener l (proxy [java.awt.event.MouseMotionListener] []
        (mouseMoved [e] )
        (mouseDragged [e]
            (.move l 
             (+ (.getX l) (.getX e)) 
             (+ (.getY l) (.getY e)) 
             )
            (.move c 
             (+ (.getX c) (.getX e)) 
             (+ (.getY c) (.getY e)) 
             )
            (let [x (.getX l)   y (.getY l)   w (.getWidth panel)  h (.getHeight panel)]
             (when (not (in-unused-area? x y))
              (toucher-moved i 
               (int (/ (* (- x (:left field-margins)) 1024) (- w (+ (:right  field-margins) (:left field-margins)))))
               (int (/ (* (- y (:top  field-margins)) 1024) (- h (+ (:bottom field-margins) (:top  field-margins)))))
              )
            ))
        )
    ))
   l)) (range num-touchers))
  ]
  (doto frame 
   (.setSize  800 550)
   (.setContentPane panel)
   (.setTitle "Virtual Touchscreen")
  )
  (doto panel
   (.setLayout nil)
  )
  (doall (map (fn[i]
      (.setBounds (nth labels i) 
        (+ (:label-x touchers-starting-position) (* (:x next-toucher-increment) i))
        (+ (:label-y touchers-starting-position) (* (:y next-toucher-increment) i))
        (:label-width touchers-sizes) 
        (:label-height touchers-sizes) 
      )
      (.setBounds (nth checkboxes i) 
        (+ (:check-x touchers-starting-position) (* (:x next-toucher-increment) i))
        (+ (:check-y touchers-starting-position) (* (:y next-toucher-increment) i))
        (:check-width touchers-sizes) 
        (:check-height touchers-sizes) 
      )
      (.add panel (nth checkboxes i))
      (.add panel (nth labels i))
    ) (range num-touchers)))
  (.setLocationRelativeTo frame nil)
  (.addWindowListener frame (proxy [java.awt.event.WindowAdapter] [] (windowClosing [_] (System/exit 0))))
  frame))





(defn listen-the-socket [port, events]
 (let [
  ss (ServerSocket. port)
  ]
    (loop []
     (let [
      s  (.accept ss)
      os (.getOutputStream s)
      p  (java.io.PrintWriter. os)
      ]
      (.setTcpNoDelay s true)
      (.start (Thread. (fn[]
        (try 
         (loop []
          (locking events
           (.wait events))
          (.print p @events)
          (.flush p)
          (recur))
        (finally (.close s) (println "fin\n")))
     ))))
    (recur))
 ))



(defn -main [] 
 (let [events (atom "")]
  (SwingUtilities/invokeLater (fn [](.show (create-touchscreen-window events))))
  (listen-the-socket 9494 events)))


(-main) ;; For starting as executable script
